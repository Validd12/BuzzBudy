package com.example.smarthomebt;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_ENABLE_BT = 1;
    private static final int REQUEST_LOCATION_PERMISSION = 2;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket bluetoothSocket;
    private BluetoothDevice bluetoothDevice;

    private TextView tvStatus, tvResponse;
    private Button btnConnect, btnAlarmOn, btnAlarmOff, btnBacklightOn, btnBacklightOff, btnServoOpen, btnServoClose;

    private OutputStream outputStream;
    private InputStream inputStream;

    private Handler handler = new Handler();
    private final UUID BT_MODULE_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // Standard SerialPortService ID

    private boolean stopWorker;
    private Thread workerThread;
    private byte[] readBuffer;
    private int readBufferPosition;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

         bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

         tvStatus = findViewById(R.id.tvStatus);
         tvResponse = findViewById(R.id.tvResponse);
         btnConnect = findViewById(R.id.btnConnect);
         btnAlarmOn = findViewById(R.id.btnAlarmOn);
         btnAlarmOff = findViewById(R.id.btnAlarmOff);
         btnBacklightOn = findViewById(R.id.btnBacklightOn);
         btnBacklightOff = findViewById(R.id.btnBacklightOff);
         btnServoOpen = findViewById(R.id.btnServoOpen);
         btnServoClose = findViewById(R.id.btnServoClose);

         btnConnect.setOnClickListener(v -> connectBluetooth());
         btnAlarmOn.setOnClickListener(v -> sendCommand("ALARM_ON\n"));
         btnAlarmOff.setOnClickListener(v -> sendCommand("ALARM_OFF\n"));
         btnBacklightOn.setOnClickListener(v -> sendCommand("BACKLIGHT_ON\n"));
         btnBacklightOff.setOnClickListener(v -> sendCommand("BACKLIGHT_OFF\n"));
         btnServoOpen.setOnClickListener(v -> sendCommand("SERVO_OPEN\n"));
         btnServoClose.setOnClickListener(v -> sendCommand("SERVO_CLOSE\n"));

         checkPermissions();
    }

    private void checkPermissions() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
            || ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED
            || ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {

            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN},
                    REQUEST_LOCATION_PERMISSION);
        }
    }

    private void connectBluetooth() {
        if (bluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth not supported", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            return;
        }

        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                if (device.getName().equals("HC-05")) {
                    bluetoothDevice = device;
                    break;
                }
            }
        }

        if (bluetoothDevice == null) {
            Toast.makeText(this, "HC-05 device not found. Pair it first.", Toast.LENGTH_LONG).show();
            return;
        }

        try {
            bluetoothSocket = bluetoothDevice.createRfcommSocketToServiceRecord(BT_MODULE_UUID);
            bluetoothSocket.connect();
            outputStream = bluetoothSocket.getOutputStream();
            inputStream = bluetoothSocket.getInputStream();
            tvStatus.setText("Connected to HC-05");
            beginListenForData();
        } catch (IOException e) {
            tvStatus.setText("Connection Failed: " + e.getMessage());
        }
    }

    private void beginListenForData() {
        stopWorker = false;
        readBufferPosition = 0;
        readBuffer = new byte[1024];

        workerThread = new Thread(() -> {
            while (!Thread.currentThread().isInterrupted() && !stopWorker) {
                try {
                    int bytesAvailable = inputStream.available();
                    if (bytesAvailable > 0) {
                        byte[] packetBytes = new byte[bytesAvailable];
                        inputStream.read(packetBytes);
                        for (int i = 0; i < bytesAvailable; i++) {
                            byte b = packetBytes[i];
                            if (b == '\n') {
                                byte[] encodedBytes = new byte[readBufferPosition];
                                System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                final String data = new String(encodedBytes, "US-ASCII");
                                readBufferPosition = 0;
                                handler.post(() -> {
                                    tvResponse.append(data + "\n");
                                    final ScrollView scrollView = (ScrollView) tvResponse.getParent();
                                    scrollView.post(() -> scrollView.fullScroll(View.FOCUS_DOWN));
                                });
                            } else {
                                readBuffer[readBufferPosition++] = b;
                            }
                        }
                    }
                } catch (IOException ex) {
                    stopWorker = true;
                }
            }
        });
        workerThread.start();
    }

    private void sendCommand(String cmd) {
        if (outputStream != null) {
            try {
                outputStream.write(cmd.getBytes());
                Toast.makeText(this, "Sent: " + cmd.trim(), Toast.LENGTH_SHORT).show();
            } catch (IOException e) {
                Toast.makeText(this, "Send failed: " + e.getMessage(), Toast.LENGTH_SHORT).show();
            }
        } else {
            Toast.makeText(this, "Not connected", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopWorker = true;
        try {
            if (bluetoothSocket != null) bluetoothSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                connectBluetooth();
            } else {
                Toast.makeText(this, "Bluetooth required", Toast.LENGTH_SHORT).show();
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}

package com.example.smarthomebt;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
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
    private final UUID BT_MODULE_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED ||
                    ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN},
                        REQUEST_LOCATION_PERMISSION);
            }
        } else {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                        REQUEST_LOCATION_PERMISSION);
            }
        }
    }

    @SuppressLint("MissingPermission")
    private
package com.example.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.Manifest
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import com.google.android.gms.location.FusedLocationProviderClient
import com.google.android.gms.location.LocationServices
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    // Native JNI method declarations
    private external fun fetchAmenitiesPlainTextNative(lat: Double, lon: Double, radius: Double): String
    private external fun getIpLocationNative(): DoubleArray
    private external fun getGpsLocationNative(): DoubleArray
    private external fun updateGpsLocationNative(lat: Double, lon: Double)

    private lateinit var fusedLocationClient: FusedLocationProviderClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this)

        val etLat = findViewById<EditText>(R.id.etLat)
        val etLon = findViewById<EditText>(R.id.etLon)
        val etRadius = findViewById<EditText>(R.id.etRadius)
        val btnFetch = findViewById<Button>(R.id.btnFetch)
        val btnIpLocation = findViewById<Button>(R.id.btnIpLocation)
        val btnGpsLocation = findViewById<Button>(R.id.btnGpsLocation)
        val tvConsole = findViewById<TextView>(R.id.tvConsole)

        btnFetch.setOnClickListener {
            val lat = etLat.text.toString().toDoubleOrNull() ?: 0.0
            val lon = etLon.text.toString().toDoubleOrNull() ?: 0.0
            val radius = etRadius.text.toString().toDoubleOrNull() ?: 1000.0

            tvConsole.text = "getting data!\n"

            thread {
                val plainTextResult = fetchAmenitiesPlainTextNative(lat, lon, radius)
                runOnUiThread {
                    tvConsole.text = plainTextResult
                }
            }
        }

        btnIpLocation.setOnClickListener {

            thread {
                val result = getIpLocationNative()
                val lat = result[0]
                val lon = result[1]
                val success = result[2] == 1.0

                runOnUiThread {
                    if (success) {
                        etLat.setText(lat.toString())
                        etLon.setText(lon.toString())
                        tvConsole.text = "IP location found\n"
                    } else {
                        tvConsole.text = "failed to get IP location.\n"
                    }
                }
            }
        }

        btnGpsLocation.setOnClickListener {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.ACCESS_FINE_LOCATION), 1001)
                return@setOnClickListener
            }

            fusedLocationClient.lastLocation.addOnSuccessListener { location ->
                if (location != null) {
                    val lat = location.latitude
                    val lon = location.longitude
                    
                    // Update native LocationService
                    thread {
                        updateGpsLocationNative(lat, lon)
                        // Then fetch it back just to prove the "feature" in locationservice.cpp works
                        val result = getGpsLocationNative()
                        val nativeLat = result[0]
                        val nativeLon = result[1]
                        
                        runOnUiThread {
                            etLat.setText(nativeLat.toString())
                            etLon.setText(nativeLon.toString())
                            tvConsole.text = "GPS location found\n"
                        }
                    }
                } else {
                    tvConsole.text = "failed to get GPS location\n"
                }
            }
        }
    }
}

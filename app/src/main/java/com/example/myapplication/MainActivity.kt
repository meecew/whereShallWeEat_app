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
import android.view.MotionEvent
import android.view.inputmethod.InputMethodManager
import android.content.Context
import android.text.Html
import android.text.method.LinkMovementMethod
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    // Native JNI method declarations
    private external fun fetchAmenitiesPlainTextNative(lat: Double, lon: Double, radius: Double): String
    private external fun fetchRandomAmenityNative(lat: Double, lon: Double, radius: Double): String
    private external fun getIpLocationNative(): DoubleArray
    private external fun getGpsLocationNative(): DoubleArray
    private external fun updateGpsLocationNative(lat: Double, lon: Double)

    private lateinit var fusedLocationClient: FusedLocationProviderClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Enable Immersive Mode (hide navigation and status bars)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
            val controller = window.insetsController
            if (controller != null) {
                controller.hide(android.view.WindowInsets.Type.statusBars() or android.view.WindowInsets.Type.navigationBars())
                controller.systemBarsBehavior = android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    or android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    or android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    or android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    or android.view.View.SYSTEM_UI_FLAG_FULLSCREEN)
        }

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this)

        val etLat = findViewById<EditText>(R.id.etLat)
        val etLon = findViewById<EditText>(R.id.etLon)
        val etRadius = findViewById<EditText>(R.id.etRadius)
        val btnFetch = findViewById<Button>(R.id.btnFetch)
        val btnIpLocation = findViewById<Button>(R.id.btnIpLocation)
        val btnGpsLocation = findViewById<Button>(R.id.btnGpsLocation)
        val btnRandom = findViewById<Button>(R.id.btnRandom)
        val tvConsole = findViewById<TextView>(R.id.tvConsole)
        
        // Allow clicking on restaurant links
        tvConsole.movementMethod = LinkMovementMethod.getInstance()

        btnFetch.setOnClickListener {
            val lat = etLat.text.toString().toDoubleOrNull() ?: 0.0
            val lon = etLon.text.toString().toDoubleOrNull() ?: 0.0
            val radius = etRadius.text.toString().toDoubleOrNull() ?: 1000.0

            tvConsole.text = "getting data!\n"

            thread {
                val plainTextResult = fetchAmenitiesPlainTextNative(lat, lon, radius)
                runOnUiThread {
                    tvConsole.text = Html.fromHtml(plainTextResult.replace("\n", "<br>"), Html.FROM_HTML_MODE_LEGACY)
                }
            }
        }

        btnRandom.setOnClickListener {
            val lat = etLat.text.toString().toDoubleOrNull() ?: 0.0
            val lon = etLon.text.toString().toDoubleOrNull() ?: 0.0
            val radius = etRadius.text.toString().toDoubleOrNull() ?: 1000.0

            tvConsole.text = "picking for you...\n"

            thread {
                val result = fetchRandomAmenityNative(lat, lon, radius)
                runOnUiThread {
                    tvConsole.text = Html.fromHtml(result.replace("\n", "<br>"), Html.FROM_HTML_MODE_LEGACY)
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
                        tvConsole.text = "IP location updated!!\n"
                    } else {
                        tvConsole.text = "failed to get IP location :(\n"
                    }
                }
            }
        }

        btnGpsLocation.setOnClickListener {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.ACCESS_FINE_LOCATION), 1001)
                return@setOnClickListener
            }

            tvConsole.text = "getting GPS...\n"

            // Request a single fresh location update instead of just the "last known"
            val locationRequest = com.google.android.gms.location.LocationRequest.Builder(
                com.google.android.gms.location.Priority.PRIORITY_HIGH_ACCURACY, 0
            ).setMaxUpdates(1).build()

            fusedLocationClient.requestLocationUpdates(locationRequest, object : com.google.android.gms.location.LocationCallback() {
                override fun onLocationResult(locationResult: com.google.android.gms.location.LocationResult) {
                    val location = locationResult.lastLocation
                    if (location != null) {
                        val lat = location.latitude
                        val lon = location.longitude
                        
                        thread {
                            updateGpsLocationNative(lat, lon)
                            val result = getGpsLocationNative()
                            val nativeLat = result[0]
                            val nativeLon = result[1]
                            
                            runOnUiThread {
                                etLat.setText(nativeLat.toString())
                                etLon.setText(nativeLon.toString())
                                tvConsole.text = "GPS location updated!!\n"
                            }
                        }
                    } else {
                        tvConsole.text = "failed to get GPS location :(\n"
                    }
                }
            }, mainLooper)
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
        if (currentFocus != null) {
            val imm = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            imm.hideSoftInputFromWindow(currentFocus!!.windowToken, 0)
            currentFocus!!.clearFocus()
        }
        return super.dispatchTouchEvent(ev)
    }
}

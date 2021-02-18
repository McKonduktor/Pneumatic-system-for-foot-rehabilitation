package com.example.bluetoothcontrol

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.AsyncTask
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_control.*
import java.io.IOException
import java.util.*


class ControlActivity : AppCompatActivity() {

    companion object {
        var m_myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb")
        var m_bluetoothSocket: BluetoothSocket? = null
        var m_state: Int = 0
        var m_pressure: Int = 0
        var m_speed: Int = 0
        var m_time: Int = 0
        lateinit var m_progress: View
        lateinit var m_pressure_seekBar: SeekBar
        lateinit var m_speed_seekBar: SeekBar
        lateinit var m_time_seekBar: SeekBar
        lateinit var m_pressure_textView : TextView
        lateinit var m_speed_textView: TextView
        lateinit var m_time_textView: TextView
        lateinit var m_bluetoothAdapter: BluetoothAdapter
        var m_isConnected: Boolean = false
        lateinit var m_address: String
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setTitle(R.string.control_title)
        setContentView(R.layout.activity_control)

        val filter = IntentFilter()
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED)
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED)
        this.registerReceiver(m_Receiver, filter)

        m_address = intent.getStringExtra("EXTRA_ADDRESS")
        m_progress = findViewById(R.id.progressBar)
        m_pressure_seekBar = findViewById(R.id.pressure_seekBar)
        m_pressure_textView = findViewById(R.id.pressure_textView)
        m_pressure_textView.text =  getString(R.string.pressure, m_pressure_seekBar.progress)
        m_speed_seekBar = findViewById(R.id.speed_seekBar)
        m_speed_textView = findViewById(R.id.speed_textView)
        m_speed_textView.text =  getString(R.string.speed, 0) + "%"
        m_time_seekBar = findViewById(R.id.time_seekBar)
        m_time_textView = findViewById(R.id.time_textView)
        m_time_textView .text = getString(R.string.time, m_time_seekBar.progress)

        m_time = 0
        m_pressure = 0

        ConnectToDevice(this).execute()

        reconnect_Button.setOnClickListener {
            if (!m_isConnected) {
                ConnectToDevice(this).execute()
            }
        }
        disconnect_Button.setOnClickListener { disconnect() }
        start_Button.setOnClickListener {
            m_state = 1
            sendCommand()
        }
        stop_Button.setOnClickListener {
            m_state = 0
            sendCommand()
        }

        m_pressure_seekBar.setOnSeekBarChangeListener( object : SeekBar.OnSeekBarChangeListener{
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                m_pressure_textView.text = getString(R.string.pressure, progress)
                m_pressure = m_pressure_seekBar.progress
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                m_pressure = m_pressure_seekBar.progress
                sendCommand()
            }

        })

        m_speed_seekBar.setOnSeekBarChangeListener( object : SeekBar.OnSeekBarChangeListener{
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val m_speed_text = (progress - 190) * 100 / 65
                m_speed_textView.text = getString(R.string.speed, m_speed_text) + "%"
                m_speed = m_speed_seekBar.progress
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                m_speed = m_speed_seekBar.progress
                sendCommand()
            }

        })

        m_time_seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener{
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                m_time_textView.text = getString(R.string.time, progress)
                m_time = m_time_seekBar.progress
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                m_time = m_time_seekBar.progress
                sendCommand()
            }

        })
    }

    private val m_Receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            var action = intent?.action

            if (BluetoothDevice.ACTION_ACL_CONNECTED == action){
                Toast.makeText(this@ControlActivity, R.string.connected, Toast.LENGTH_LONG).show()
            }

            if (BluetoothDevice.ACTION_ACL_DISCONNECTED == action)
                Toast.makeText(this@ControlActivity, R.string.disconnected, Toast.LENGTH_LONG).show()
                m_isConnected = false
        }
    }

    private fun sendCommand() {
        var data = byteArrayOf(m_state.toByte(), m_pressure.toByte(), m_speed.toByte(), m_time.toByte())
        if(m_bluetoothSocket != null) {
            try {
                m_bluetoothSocket!!.outputStream.write(data)
                Log.d("data_bt", "sent")
            } catch (e: IOException) {
                e.printStackTrace()
                Log.d("data_bt", "not sent")
            }
        }
    }

    private fun disconnect() {
        if (m_bluetoothSocket != null) {
            try {
                m_bluetoothSocket!!.close()
                m_bluetoothSocket = null
                m_isConnected = false
            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
        finish()
    }

    private class ConnectToDevice(c: Context) : AsyncTask<Void, Void, String>() {
        private var connectSuccess: Boolean = true
        private val context: Context = c

        override fun onPreExecute() {
            super.onPreExecute()
            m_progress.visibility = View.VISIBLE

        }

        override fun doInBackground(vararg params: Void?): String? {
            try {
                if (m_bluetoothSocket == null || !m_isConnected){
                    Log.d("data_bt", "connecting")
                    m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
                    val device : BluetoothDevice = m_bluetoothAdapter.getRemoteDevice(m_address)
                    m_bluetoothSocket = device.createInsecureRfcommSocketToServiceRecord(m_myUUID)
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery()
                    m_bluetoothSocket!!.connect()
                }
            } catch (e: IOException) {
                try {
                    val device : BluetoothDevice = m_bluetoothAdapter.getRemoteDevice(m_address)
                    val clazz: Class<*> = m_bluetoothSocket!!.remoteDevice.javaClass
                    val paramTypes = arrayOf<Class<*>>(Integer.TYPE)
                    val m = clazz.getMethod("createRfcommSocket", *paramTypes)
                    val params = arrayOf<Any>(Integer.valueOf(1))
                    val fallbackSocket = m.invoke(m_bluetoothSocket!!.remoteDevice, params) as BluetoothSocket
                    fallbackSocket.connect()
                } catch (e: IOException) {
                    connectSuccess = false
                    e.printStackTrace()
                }
                connectSuccess = false
                e.printStackTrace()
            }
            return null
        }

        override fun onPostExecute(result: String?) {
            super.onPostExecute(result)
            if (!connectSuccess) {
                Log.d("data_bt", "couldn't connect")
                Toast.makeText(context, R.string.connection_error, Toast.LENGTH_LONG).show()
                context.startActivity(Intent(context, MainActivity::class.java))
            } else {
                m_isConnected = true
                Log.d("data_bt", "connected")
            }
            m_progress.visibility = View.GONE
        }
    }
}

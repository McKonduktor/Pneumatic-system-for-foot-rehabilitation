package com.example.bluetoothcontrol

import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    private var m_bluetoothAdapter: BluetoothAdapter? = null
    private lateinit var m_pairedDevices: Set<BluetoothDevice>
    private val REQUEST_ENABLE_BLUETOOTH = 1

    companion object {
        val EXTRA_ADDRESS: String = "Device_address"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setTitle(R.string.main_title)
        setContentView(R.layout.activity_main)

        m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        if (m_bluetoothAdapter == null) {
            Toast.makeText(this, R.string.no_bluetooth, Toast.LENGTH_LONG).show()
            return
        }
        if(!m_bluetoothAdapter!!.isEnabled){
            val enableBluetoothIntent = Intent (BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(enableBluetoothIntent, REQUEST_ENABLE_BLUETOOTH)
        }
        pairedDeviceList()
        select_device_refresh.setOnClickListener{ pairedDeviceList() }

    }

    private fun pairedDeviceList() {
        m_pairedDevices = m_bluetoothAdapter!!.bondedDevices
        val list: ArrayList<BluetoothDevice> = ArrayList()
        val name_list: ArrayList<String> = ArrayList()

        if (m_pairedDevices.isNotEmpty()) {
            for (device: BluetoothDevice in m_pairedDevices) {
                list.add(device)
                name_list.add(device.name)
                Log.i("device", ""+device)
            }
        } else {
            Toast.makeText(this, R.string.no_paired_devices_found, Toast.LENGTH_LONG).show()
        }

        val adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, name_list)
        select_device_list.adapter = adapter
        select_device_list.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
            val device: BluetoothDevice = list[position]
            val address: String = device.address
            val intent = Intent (this, ControlActivity::class.java)
            intent.putExtra("EXTRA_ADDRESS", address)
            startActivity(intent)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_ENABLE_BLUETOOTH) {
            if (resultCode == Activity.RESULT_OK) {
                if (m_bluetoothAdapter!!.isEnabled) {
                    Toast.makeText(this, R.string.bluetooth_enabled, Toast.LENGTH_LONG).show()
                } else {
                    Toast.makeText(this, R.string.bluetooth_disabled, Toast.LENGTH_LONG).show()
                }
            } else if (resultCode == Activity.RESULT_CANCELED) {
                Toast.makeText(this, R.string.bluetooth_canceled, Toast.LENGTH_LONG).show()
            }
        }
    }

}

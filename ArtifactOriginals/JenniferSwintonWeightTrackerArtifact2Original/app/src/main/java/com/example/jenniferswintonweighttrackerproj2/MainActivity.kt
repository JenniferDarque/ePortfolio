package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.activity.ComponentActivity

class MainActivity : ComponentActivity() {

    private lateinit var dbHelper: DatabaseHelper

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)

        dbHelper = DatabaseHelper(this)

        val usernameInput = findViewById<EditText>(R.id.usernameInput)
        val passwordInput = findViewById<EditText>(R.id.editTextTextPassword)
        val loginButton = findViewById<Button>(R.id.loginButton)
        val createAccountButton = findViewById<Button>(R.id.createAccountButton)
        val loginErrorMessage = findViewById<TextView>(R.id.loginErrorMessage)

        loginButton.setOnClickListener {
            val username = usernameInput.text.toString().trim()
            val password = passwordInput.text.toString().trim()

            if (username.isNotEmpty() && password.isNotEmpty()) {
                // Check credentials in SQLite Database
                if (dbHelper.checkUser(username, password)) {
                    // Store the current logged-in username
                    val userPrefs = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
                    userPrefs.edit().putString("current_username", username).apply()

                    val intent = Intent(this, WeightTrendActivity::class.java)
                    startActivity(intent)
                    loginErrorMessage.visibility = android.view.View.GONE
                } else {
                    loginErrorMessage.visibility = android.view.View.VISIBLE
                }
            } else {
                loginErrorMessage.visibility = android.view.View.VISIBLE
            }
        }

        createAccountButton.setOnClickListener {
            val intent = Intent(this, CreateAccountActivity::class.java)
            startActivity(intent)
        }
    }
}

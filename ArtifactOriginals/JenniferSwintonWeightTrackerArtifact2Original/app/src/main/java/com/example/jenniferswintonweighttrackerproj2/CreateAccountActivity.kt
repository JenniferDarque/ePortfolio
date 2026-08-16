package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity

class CreateAccountActivity : ComponentActivity() {

    private val errorHandler = Handler(Looper.getMainLooper())
    private var errorRunnable: Runnable? = null
    private lateinit var dbHelper: DatabaseHelper

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_create_account)

        dbHelper = DatabaseHelper(this)

        val etNewUsername = findViewById<EditText>(R.id.etNewUsername)
        val etNewPassword = findViewById<EditText>(R.id.etNewPassword)
        val etConfirmPassword = findViewById<EditText>(R.id.etConfirmPassword)
        val btnSubmit = findViewById<Button>(R.id.btnSubmitCreateAccount)
        val btnCancel = findViewById<Button>(R.id.btnCancelCreateAccount)
        val tvError = findViewById<TextView>(R.id.tvCreateAccountError)

        btnSubmit.setOnClickListener {
            val username = etNewUsername.text.toString().trim()
            val password = etNewPassword.text.toString().trim()
            val confirmPassword = etConfirmPassword.text.toString().trim()

            tvError.visibility = View.GONE
            errorRunnable?.let { errorHandler.removeCallbacks(it) }

            if (username.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
                showError(getString(R.string.fields_cannot_be_empty))
                return@setOnClickListener
            }

            // Insecure password check
            if (password == "password" || password == "123456") {
                showError(getString(R.string.insecure_password))
                
                // Keep the error message visible for 7 seconds
                errorRunnable = Runnable {
                    tvError.visibility = View.GONE
                }
                errorHandler.postDelayed(errorRunnable!!, 7000)
                
                return@setOnClickListener
            }

            // Username validation: 5-20 characters, letters and numbers only
            val usernameRegex = "^[a-zA-Z0-9]{5,20}$".toRegex()
            if (!username.matches(usernameRegex)) {
                showError(getString(R.string.invalid_username_format))
                return@setOnClickListener
            }

            // Password validation: 8-25 characters, letters, numbers, and @!$#%&
            val passwordRegex = "^[a-zA-Z0-9@!$#%&]{8,25}$".toRegex()
            if (!password.matches(passwordRegex)) {
                showError(getString(R.string.invalid_password_format))
                return@setOnClickListener
            }

            if (password != confirmPassword) {
                showError(getString(R.string.passwords_do_not_match))
                return@setOnClickListener
            }

            // Save credentials to SQLite Database
            val result = dbHelper.addUser(username, password)
            if (result != -1L) {
                Toast.makeText(this, getString(R.string.account_created), Toast.LENGTH_SHORT).show()
                finish() // Go back to login screen
            } else {
                showError("Username already exists. Please choose another.")
            }
        }

        btnCancel.setOnClickListener {
            finish() // Simply closes this activity and returns to MainActivity (Login)
        }
    }

    private fun showError(message: String) {
        val tvError = findViewById<TextView>(R.id.tvCreateAccountError)
        tvError.text = message
        tvError.visibility = View.VISIBLE
    }

    override fun onDestroy() {
        super.onDestroy()
        errorRunnable?.let { errorHandler.removeCallbacks(it) }
    }
}

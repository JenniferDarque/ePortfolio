package com.example.jenniferswintonweighttrackerproj2

import android.graphics.Typeface
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.recyclerview.widget.RecyclerView

class WeightAdapter(
    private val weightEntries: MutableList<WeightEntry>,
    private val onDeleteClick: (WeightEntry) -> Unit,
    private val onUpdateClick: (WeightEntry, String, String) -> Unit
) : RecyclerView.Adapter<WeightAdapter.WeightViewHolder>() {

    class WeightViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val tvDate: TextView = itemView.findViewById(R.id.tvDate)
        val tvWeight: TextView = itemView.findViewById(R.id.tvWeight)
        val btnDelete: Button = itemView.findViewById(R.id.btnDelete)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): WeightViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.recycler_item, parent, false)
        return WeightViewHolder(view)
    }

    override fun onBindViewHolder(holder: WeightViewHolder, position: Int) {
        val entry = weightEntries[position]
        holder.tvDate.text = entry.date
        holder.tvWeight.text = entry.weight
        
        // Requirement: The last entered weight should display bold.
        // Since the list is sorted with the most recent date/ID at the top (index 0).
        if (position == 0) {
            holder.tvDate.setTypeface(null, Typeface.BOLD)
            holder.tvWeight.setTypeface(null, Typeface.BOLD)
        } else {
            holder.tvDate.setTypeface(null, Typeface.NORMAL)
            holder.tvWeight.setTypeface(null, Typeface.NORMAL)
        }
        
        holder.btnDelete.setOnClickListener {
            onDeleteClick(entry)
        }

        holder.itemView.setOnClickListener {
            showUpdateDialog(holder.itemView.context, entry)
        }
    }

    private fun showUpdateDialog(context: android.content.Context, entry: WeightEntry) {
        val builder = AlertDialog.Builder(context)
        builder.setTitle("Update Entry")

        val view = LayoutInflater.from(context).inflate(R.layout.dialog_update_entry, null)
        val etDate = view.findViewById<EditText>(R.id.etUpdateDate)
        val etWeight = view.findViewById<EditText>(R.id.etUpdateWeight)

        etDate.setText(entry.date)
        etWeight.setText(entry.weight)

        // Attach date auto-formatter ONLY to the date field
        setupDateAutoFormatter(etDate)

        builder.setView(view)
        builder.setPositiveButton("Update") { _, _ ->
            val newDate = etDate.text.toString()
            val newWeight = etWeight.text.toString()
            if (newDate.isNotEmpty() && newWeight.isNotEmpty()) {
                onUpdateClick(entry, newDate, newWeight)
            }
        }
        builder.setNegativeButton("Cancel", null)
        builder.show()
    }

    private fun setupDateAutoFormatter(editText: EditText) {
        editText.addTextChangedListener(object : TextWatcher {
            private var isUpdating = false
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
            override fun afterTextChanged(s: Editable?) {
                if (isUpdating) return
                isUpdating = true

                val input = s.toString().replace("/", "")
                val formatted = StringBuilder()
                for (i in input.indices) {
                    if (i == 2 || i == 4) {
                        formatted.append("/")
                    }
                    formatted.append(input[i])
                    if (formatted.length >= 10) break
                }

                if (formatted.toString() != s.toString()) {
                    s?.replace(0, s.length, formatted.toString())
                }
                isUpdating = false
            }
        })
    }

    override fun getItemCount(): Int = weightEntries.size
}

package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Typeface
import android.util.AttributeSet
import android.view.View

class GraphView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var dataPoints: List<Double> = emptyList()
    private var isFallback: Boolean = false
    
    private val linePaint = Paint().apply {
        color = Color.BLUE
        strokeWidth = 5f
        style = Paint.Style.STROKE
        isAntiAlias = true
    }

    private val pointPaint = Paint().apply {
        color = Color.RED
        style = Paint.Style.FILL
        isAntiAlias = true
    }

    private val axisPaint = Paint().apply {
        color = Color.BLACK
        strokeWidth = 3f
        style = Paint.Style.STROKE
    }

    private val textPaint = Paint().apply {
        color = Color.parseColor("#00008B") // Dark Blue
        textSize = 40f
        isAntiAlias = true
        typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        textAlign = Paint.Align.CENTER
    }

    private val labelPaint = Paint().apply {
        color = Color.BLACK
        textSize = 30f
        isAntiAlias = true
        typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        textAlign = Paint.Align.CENTER
    }

    private val valuePaint = Paint().apply {
        color = Color.DKGRAY
        textSize = 24f
        isAntiAlias = true
        textAlign = Paint.Align.RIGHT
    }

    fun setData(newData: List<Double>, fallback: Boolean = false) {
        dataPoints = newData
        isFallback = fallback
        invalidate() // Redraw the view
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        
        if (isFallback) {
            canvas.drawText("No entries for this time. Showing all entries", (width / 2).toFloat(), 60f, textPaint)
        }

        val padding = 120f
        val graphWidth = width - 2 * padding
        val graphHeight = height - 2 * padding

        // Draw Axes
        canvas.drawLine(padding, padding, padding, height - padding, axisPaint) // Y
        canvas.drawLine(padding, height - padding, width - padding, height - padding, axisPaint) // X

        // X-Axis Label
        canvas.drawText("Time (Recent → Today)", width / 2f, height - 30f, labelPaint)

        // Y-Axis Label (Rotated)
        canvas.save()
        canvas.rotate(-90f, 40f, height / 2f)
        canvas.drawText("Weight (lbs)", 40f, height / 2f, labelPaint)
        canvas.restore()

        if (dataPoints.isEmpty()) return

        val maxVal = dataPoints.maxOrNull() ?: 1.0
        val minVal = dataPoints.minOrNull() ?: 0.0
        val range = if (maxVal == minVal) 1.0 else maxVal - minVal

        // Draw Min/Max values on Y-axis
        canvas.drawText("%.1f".format(maxVal), padding - 10f, padding + 10f, valuePaint)
        canvas.drawText("%.1f".format(minVal), padding - 10f, height - padding, valuePaint)

        val stepX = if (dataPoints.size > 1) graphWidth / (dataPoints.size - 1) else 0f
        
        val path = Path()
        
        for (i in dataPoints.indices) {
            val x = padding + i * stepX
            val y = height - padding - ((dataPoints[i] - minVal) / range * graphHeight).toFloat()

            if (i == 0) {
                path.moveTo(x, y)
            } else {
                path.lineTo(x, y)
            }
            canvas.drawCircle(x, y, 8f, pointPaint)
        }
        
        canvas.drawPath(path, linePaint)
    }
}

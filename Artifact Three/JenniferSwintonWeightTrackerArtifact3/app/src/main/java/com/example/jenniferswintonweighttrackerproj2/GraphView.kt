package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.DashPathEffect
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Typeface
import android.util.AttributeSet
import android.view.View
import androidx.core.graphics.toColorInt
import androidx.core.graphics.withRotation

class GraphView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var historicalPoints: List<Double> = emptyList()
    private var predictivePoints: List<Double> = emptyList()
    private var isFallback: Boolean = false

    private val historicalPath = Path()
    private val predictivePath = Path()
    
    private val linePaint = Paint().apply {
        color = Color.BLUE
        strokeWidth = 5f
        style = Paint.Style.STROKE
        isAntiAlias = true
    }

    private val predictiveLinePaint = Paint().apply {
        color = Color.BLUE
        strokeWidth = 5f
        style = Paint.Style.STROKE
        isAntiAlias = true
        pathEffect = DashPathEffect(floatArrayOf(10f, 10f), 0f)
    }

    private val pointPaint = Paint().apply {
        color = Color.RED
        style = Paint.Style.FILL
        isAntiAlias = true
    }

    private val predictivePointPaint = Paint().apply {
        color = "#FF8080".toColorInt() // Light Red
        style = Paint.Style.FILL
        isAntiAlias = true
    }

    private val axisPaint = Paint().apply {
        color = Color.BLACK
        strokeWidth = 3f
        style = Paint.Style.STROKE
    }

    private val textPaint = Paint().apply {
        color = "#00008B".toColorInt() // Dark Blue
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

    fun setData(historical: List<Double>, predictive: List<Double> = emptyList(), fallback: Boolean = false) {
        historicalPoints = historical
        predictivePoints = predictive
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
        canvas.drawText("Time (History → Prediction)", width / 2f, height - 30f, labelPaint)

        // Y-Axis Label (Rotated)
        canvas.withRotation(-90f, 40f, height / 2f) {
            drawText("Weight (lbs)", 40f, height / 2f, labelPaint)
        }

        val allPoints = historicalPoints + predictivePoints
        if (allPoints.isEmpty()) return

        val maxVal = allPoints.maxOrNull() ?: 1.0
        val minVal = allPoints.minOrNull() ?: 0.0
        val range = if (maxVal == minVal) 1.0 else maxVal - minVal

        // Draw Min/Max values on Y-axis
        canvas.drawText("%.1f".format(maxVal), padding - 10f, padding + 10f, valuePaint)
        canvas.drawText("%.1f".format(minVal), padding - 10f, height - padding, valuePaint)

        val totalSize = allPoints.size
        val stepX = if (totalSize > 1) graphWidth / (totalSize - 1) else 0f
        
        // 1. Draw Historical Path
        if (historicalPoints.isNotEmpty()) {
            historicalPath.reset()
            for (i in historicalPoints.indices) {
                val x = padding + i * stepX
                val y = height - padding - ((historicalPoints[i] - minVal) / range * graphHeight).toFloat()
                if (i == 0) historicalPath.moveTo(x, y) else historicalPath.lineTo(x, y)
                canvas.drawCircle(x, y, 8f, pointPaint)
            }
            canvas.drawPath(historicalPath, linePaint)
        }

        // 2. Draw Predictive Path
        if (predictivePoints.isNotEmpty() && historicalPoints.isNotEmpty()) {
            predictivePath.reset()
            // Start from the last historical point
            val lastHistIdx = historicalPoints.size - 1
            val startX = padding + lastHistIdx * stepX
            val startY = height - padding - ((historicalPoints.last() - minVal) / range * graphHeight).toFloat()
            predictivePath.moveTo(startX, startY)

            for (i in predictivePoints.indices) {
                val x = padding + (lastHistIdx + 1 + i) * stepX
                val y = height - padding - ((predictivePoints[i] - minVal) / range * graphHeight).toFloat()
                predictivePath.lineTo(x, y)
                canvas.drawCircle(x, y, 8f, predictivePointPaint)
            }
            canvas.drawPath(predictivePath, predictiveLinePaint)
        }
    }
}

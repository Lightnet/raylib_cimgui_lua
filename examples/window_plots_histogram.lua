-- window

local values = {0.1, 0.5, 0.9, 0.2, 0.8, 0.4}

function draw()
    imgui.Begin("Lua UI")
    imgui.Text("Hello from Lua!")
    imgui.PlotHistogram("Histogram", values, "Histogram", 0.0, 1.0, 200, 100)
    imgui.End()
end

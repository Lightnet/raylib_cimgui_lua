-- window 

local radio_value = 1

function draw()

    imgui.Begin("Test")
    imgui.TextColored({0.0, 1.0, 1.0, 1.0}, "Progress")
    imgui.ProgressBar(0.75, -1.0, 0.0, "75%")
    if imgui.IsItemHovered() then
        imgui.BeginTooltip()
        imgui.Text("Progress at 75%")
        imgui.EndTooltip()
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end
-- window

local slider_value = 50.0

function draw()
    imgui.Begin("Lua UI")
    local changed, new_value = imgui.VSliderFloat("##slider", slider_value, 0.0, 100.0,nil, nil, "%.1f")
    if changed then
        print("Slider value:", new_value)
    end
    imgui.End()
end

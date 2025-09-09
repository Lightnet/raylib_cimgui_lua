-- window Slider

local window_open = true
local slider_value = 0.5

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open, {}) then
        imgui.Text("Hello from Lua!")
        
        -- Slider example: Pass value, receive updated value and changed flag
        local new_slider, changed1 = imgui.SliderFloat("Slider Value", slider_value, 0.0, 1.0, "%.2f", 0)
        if changed1 then
            slider_value = new_slider  -- Update local variable
            print("Slider changed to: " .. slider_value)
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end
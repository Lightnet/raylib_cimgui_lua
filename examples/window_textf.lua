-- window_textf.lua

-- Check ImGui version
local version = imgui.GetVersion()
print("ImGui version: " .. version)

-- Declare variables directly as Lua numbers/booleans
local slider_value = 0.5
local window_open = true

function draw()
    -- Begin main window
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua!")
        
        -- Slider example: Pass value, receive updated value and changed flag
        local new_slider, changed1 = imgui.SliderFloat("Slider Value", slider_value, 0.0, 1.0, "%.2f", 0)
        if changed1 then
            slider_value = new_slider  -- Update local variable
            print("Slider changed to: " .. slider_value)
        end
        
        -- Formatted text example (note: current textf binding is limited; use string.format for complex cases)
        imgui.Textf("Value: %.2f", slider_value)
    end
    imgui.End()
    
end
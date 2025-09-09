-- example.lua
-- Simple ImGui demo using the simplified Lua bindings

-- Apply dark style (call once at startup)
imgui.style_colors_dark()

-- Check ImGui version
local version = imgui.get_version()
print("ImGui version: " .. version)

-- Declare variables directly as Lua numbers/booleans
local slider_value = 0.5
local checkbox_value = false
local window_open = true

function draw()
    -- Begin main window
    if imgui.begin_window("Lua ImGui Demo", window_open, 0) then
        imgui.text("Hello from Lua!")
        
        -- Slider example: Pass value, receive updated value and changed flag
        local new_slider, changed1 = imgui.slider_float("Slider Value", slider_value, 0.0, 1.0, "%.2f", 0)
        if changed1 then
            slider_value = new_slider  -- Update local variable
            print("Slider changed to: " .. slider_value)
        end
        
        -- Checkbox example: Pass value, receive updated value and changed flag
        local new_checkbox, changed2 = imgui.checkbox("Enable Feature", checkbox_value)
        if changed2 then
            checkbox_value = new_checkbox  -- Update local variable
            print("Checkbox changed to: " .. tostring(checkbox_value))
        end
        
        -- Button example
        if imgui.button("Click Me", 0, 0) then
            print("Button pressed!")
        end
        
        -- Formatted text example (note: current textf binding is limited; use string.format for complex cases)
        imgui.textf("Value: %.2f", slider_value)
    end
    imgui.end_window()
    
    -- Update window_open if it was passed (begin_window updates it internally if userdata was used, but since we use bool, check result)
    -- For simplicity, you can re-assign window_open based on logic if needed, but in this binding, begin_window doesn't return the open state separately.
    -- If window was closed via ImGui, you may need to handle it differently (e.g., via a separate is_window_open check or by passing a reference-like mechanism).
    
    -- Example: If you want to detect window close, you could modify begin_window to always return the open state as second return value.
end
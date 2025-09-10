-- window 

-- Set custom style colors
-- imgui.SetStyleCustom({
--     -- [imgui.Col.Text] = {1.0, 1.0, 1.0, 1.0}, -- White text
--     -- [imgui.Col.Text] = {1.0, 0.1, 0.5, 1.0}, -- White text
--     Text = {1.0, 0.1, 0.5, 1.0}, -- White text
--     -- [imgui.Col.WindowBg] = {0.2, 0.2, 0.2, 1.0}, -- Dark gray background
--     -- [imgui.Col.Button] = {0.0, 0.5, 0.0, 1.0}, -- Green button
--     -- [imgui.Col.ButtonHovered] = {0.0, 0.7, 0.0, 1.0}, -- Lighter green on hover
-- })

-- imgui.SetStyleCustom({
--     [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0}, -- White text
--     [imgui.Col.WindowBg] = {0.2, 0.2, 0.2, 1.0} -- Dark gray background
-- })

-- imgui.SetStyleCustom({ [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0} })

print("imgui.Col.Text")
print(imgui.Col.Text)

-- Get a style color
local text_color = imgui.GetStyleCustom(imgui.Col.Text)
-- local text_color = imgui.GetStyleCustom("Col_Text")
print("Text color:")
print(text_color)
-- print("Text color:", text_color[1], text_color[2], text_color[3], text_color[4])

print("Text color:", text_color and text_color[1], text_color and text_color[2], 
      text_color and text_color[3], text_color and text_color[4])

local ref_color = {0.5, 0.5, 0.5, 1.0}
local color4 = {1.0, 0.0, 0.0, 1.0} -- RGBA
function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua ColorEdit4!")
        -- ColorPicker4 with table flags and ref_col
        
        -- local new_color_picker4, changed_picker4 = imgui.ColorPicker4("RGBA Picker", color4, {
        --     imgui.ColorEditFlags.PickerHueBar,
        --     -- imgui.ColorEditFlags.NoTooltip
        -- }, ref_color)

        -- if changed_picker4 then
        --     print("Picker4 changed to:", new_color_picker4[1], new_color_picker4[2], new_color_picker4[3], new_color_picker4[4])
        --     color4 = new_color_picker4
        --     imgui.SetStyleCustom({
        --         [imgui.Col.Text] = new_color_picker4, -- White text
        --     })
        -- end

        if imgui.Button("test1") then
            print(imgui.Col.Text)
        end

        if imgui.Button("test") then
            -- imgui.SetStyleCustom({ [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0} })
            imgui.SetStyleCustom(imgui.Col.Text, {0.0, 0.0, 1.0, 1.0}) -- Set blue text
        end

        if imgui.Button("set blue") then
            -- imgui.SetStyleCustom({ [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0} })
            imgui.SetStyleCustom(imgui.Col.Text, {0.0, 0.0, 1.0, 1.0}) -- Set blue text
        end

        if imgui.Button("set white") then
            -- imgui.SetStyleCustom({ [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0} })
            imgui.SetStyleCustom(imgui.Col.Text, {1.0, 1.0, 1.0, 1.0}) -- Set blue text
        end

        if imgui.Button("set white2") then
             imgui.SetStyleCustoms({
                [imgui.Col.Text] = {0.0, 0.0, 1.0, 1.0}, -- Blue text
                [imgui.Col.WindowBg] = {0.2, 0.2, 0.2, 1.0} -- Dark gray background
            })
        end

        if imgui.Button("get test") then
            local text_color = imgui.GetStyleCustom(imgui.Col.Text)
            -- local text_color = imgui.GetStyleCustom("Col_Text")
            print("Text color:")
            print(text_color)
            print("Text color:", text_color and text_color[1], text_color and text_color[2], 
                text_color and text_color[3], text_color and text_color[4])
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end

imgui.SetStyleCustom(imgui.Col.Text, {0.0, 0.0, 1.0, 1.0})
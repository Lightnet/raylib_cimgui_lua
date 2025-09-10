-- window
local Col_Text = imgui.GetStyleCustom(imgui.Col.Text)
local Col_TextDisabled = imgui.GetStyleCustom(imgui.Col.TextDisabled)
local Col_WindowBg = imgui.GetStyleCustom(imgui.Col.WindowBg)



local color4 = {1.0, 0.0, 0.0, 1.0} -- RGBA

function draw()
    imgui.Begin("Lua UI")

    if imgui.BeginTabBar("MyTabBar") then
        local selected, open = imgui.BeginTabItem("Theme Color")
        if selected then
            imgui.Text("Content of Tab 1")

            -- local new_color_picker4_1, changed_picker4_1 = imgui.ColorPicker4("RGBA Picker", text_color, {
            --     imgui.ColorEditFlags.PickerHueBar,
            --     imgui.ColorEditFlags.NoTooltip
            -- })
            -- imgui.Col.Text


            local new_color4, changed_4_1 = imgui.ColorEdit4(imgui.Col.Text, Col_Text, {
                imgui.ColorEditFlags.NoAlpha,
                imgui.ColorEditFlags.DisplayRGB
            })
            if changed_4_1 then
                Col_Text = new_color4
                imgui.SetStyleCustom(imgui.Col.Text, new_color4) -- Set blue text
            end

            new_color4, changed_4_1 = imgui.ColorEdit4(imgui.Col.TextDisabled, Col_TextDisabled, {
                imgui.ColorEditFlags.NoAlpha,
                imgui.ColorEditFlags.DisplayRGB
            })
            if changed_4_1 then
                Col_TextDisabled = new_color4
                imgui.SetStyleCustom(imgui.Col.TextDisabled, new_color4) -- Set blue text
            end


            new_color4, changed_4_1 = imgui.ColorEdit4(imgui.Col.WindowBg, Col_WindowBg, {
                imgui.ColorEditFlags.NoAlpha,
                imgui.ColorEditFlags.DisplayRGB
            })
            if changed_4_1 then
                Col_WindowBg = new_color4
                imgui.SetStyleCustom(imgui.Col.WindowBg, new_color4) -- Set blue text
            end

            imgui.EndTabItem()
        end
        
        selected, open = imgui.BeginTabItem("Tab 2")
        if selected then
            imgui.Text("Content of Tab 2")
            imgui.EndTabItem()
        end
        
        imgui.EndTabBar()
    end

    imgui.End()
end

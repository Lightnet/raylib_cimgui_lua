-- window blank

function draw()

    imgui.Begin("Test Tabs")
    if imgui.BeginTabBar("MyTabBar") then
        local selected, open = imgui.BeginTabItem("Tab 1")
        if selected then
            imgui.Text("Content of Tab 1")
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

function cleanup()
    print("Lua cleanup called")
end
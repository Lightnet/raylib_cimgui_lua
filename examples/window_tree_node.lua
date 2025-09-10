-- window 

local radio_value = 1

function draw()

    imgui.Begin("Test")
    imgui.TextColored({1.0, 0.5, 0.0, 1.0}, "Tree View")
    if imgui.TreeNode("Root Node") then
        imgui.Bullet()
        imgui.Text("Item 1")
        if imgui.IsItemClicked() then
            print("Item 1 clicked")
        end
        imgui.Bullet()
        imgui.Text("Item 2")
        if imgui.TreeNode("Sub Node") then
            imgui.Bullet()
            imgui.Text("Sub Item")
            imgui.TreePop()
        end
        imgui.TreePop()
    end


    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end
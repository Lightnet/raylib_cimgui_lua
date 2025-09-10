-- server.lua
local server = nil

function init()
    enet.initialize()
    server = enet.host_create({host = "127.0.0.1", port = 6789}, 32, 2, 0, 0)
    if not server then
        print("Failed to create server")
        return
    end
    print("Server started on 127.0.0.1:6789")
end

function draw()
    imgui.Begin("Server", nil, {})
    imgui.Text("Server running on 127.0.0.1:6789")
    if imgui.Button("host") then
        init()
    end
    imgui.End()

    if server then
        local event = enet.host_service(server, 0)
        if type(event) == "table" then
            if event.type == enet.EVENT_TYPE_CONNECT then
                print("Client connected")
            elseif event.type == enet.EVENT_TYPE_DISCONNECT then
                print("Client disconnected")
            elseif event.type == enet.EVENT_TYPE_RECEIVE then
                print("Received: " .. enet.packet_data(event.packet))
                enet.packet_destroy(event.packet)
            end
        end
    end
end

function cleanup()
    if server then
        enet.host_destroy(server)
    end
    enet.deinitialize()
    imgui.cleanup()
end
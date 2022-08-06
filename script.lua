local count = 0
while true do
    count = count + 1
    print('[lua] triggered by lua vm thread :', count)
    coroutine.yield()
end
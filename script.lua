local count = 0
while true do
    count = count + 1
    print('[lua] triggered by lua vm thread :', count)
    print('[lua] consumed memory :', collectgarbage('count'))
    coroutine.yield()
end
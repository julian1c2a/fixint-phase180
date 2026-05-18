docker run --rm --platform linux/arm/v7 `
    -v "E:/Dropbox/GitHub/cpp/fixint-phase180:/project" `
    fixint-crosstest:arm32 bash -c "echo hello > /project/docker_test.txt && echo done"
Write-Host "Exit: $LASTEXITCODE"

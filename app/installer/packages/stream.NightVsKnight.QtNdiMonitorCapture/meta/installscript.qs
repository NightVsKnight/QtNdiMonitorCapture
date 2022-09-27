function Component()
{
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install README.txt!
    component.createOperations();

    if (systemInfo.productType === "windows") {
        console.log("creating start menu entries");
        component.addOperation("CreateShortcut",
            "@TargetDir@\\QtNdiMonitorCapture.exe",
            "@StartMenuDir@\\QtNdiMonitorCapture.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@\\QtNdiMonitorCapture.exe",
            "iconId=0",
            "description=QtNdiMonitorCapture");
    }
}

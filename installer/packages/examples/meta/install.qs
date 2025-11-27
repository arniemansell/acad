function Component()
{}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/examples", "@StartMenuDir@/Examples.lnk",
            "workingDirectory=@TargetDir@", "iconPath=%systemroot%\system32\shell32.dll",
            "iconId=2", "description=ACAD example designs");
    }
}
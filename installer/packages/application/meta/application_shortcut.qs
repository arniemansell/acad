function Component()
{}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/acad.exe", "@StartMenuDir@/ACAD Wing Design.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/mum.ico",
            "description=ACAD Application");
    }
}
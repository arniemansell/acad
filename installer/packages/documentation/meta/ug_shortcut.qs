function Component()
{}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/User Guide.pdf", "@StartMenuDir@/ACAD User Guide.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/mum.ico",
            "description=ACAD User Guide");
    }
}
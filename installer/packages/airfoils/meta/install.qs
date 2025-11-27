function Component()
{}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/airfoils", "@StartMenuDir@/Airfoils.lnk",
            "workingDirectory=@TargetDir@", "iconPath=%systemroot%\system32\shell32.dll",
            iconId=2, "description=Airfoil definition files");
    }
}
Build Troubleshooting
======================
Cannot locate "\stampinf.exe"
---------------------------
> 1> C:\Program Files (x86)\Windows Kits\10\build\WindowsDriver.common.targets(1264,5): error MSB6004: The specified task executable location "\stampinf.exe" is invalid.

This might occur since build variables 'WDKBinRoot_x86' and/or 'WDKBinRoot_x64' are undefined.

To fix this, we will be modifying file "C:\Program Files (x86)\Windows Kits\10\build\WindowsDriver.Shared.Props".  
**You are advised to make a backup of this file before changes.**

At the top of the file, add a \<PropertyGroup\> like so:
```xml
<!-- Beginning of file -->
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <!-- Add below <PropertyGroup>. Context above. -->

  <PropertyGroup>
    <WDKBinRoot Condition="'$(WDKBinRoot)' == ''">$(WindowsSdkDir)bin</WDKBinRoot>
    <WDKBinRoot_x86>$(WDKBinRoot)\x86</WDKBinRoot_x86>
    <WDKBinRoot_x64>$(WDKBinRoot)\x64</WDKBinRoot_x64>
  </PropertyGroup>

  <!-- Add above <PropertyGroup>. Context below. -->
  <PropertyGroup>
    <!-- ... -->
  </PropertyGroup>
```

Source: [Forum reply] adonais-msft, "VS2015 and VS2017 side by side - RS3 WDK installation," https://social.msdn.microsoft.com/Forums/vstudio/en-US/4dd3e882-b710-41e2-9d58-a9fb68219670/vs2015-and-vs2017-side-by-side-rs3-wdk-installation?forum=wdk , accessed- uh- mid-March-ish?

inf2cat.exe failed unexpectedly
--------------------------------
> Inf2Cat, unknown failure.

> inf2catOutput.log:  
> Unhandled Exception: System.IO.FileNotFoundException: Could not load file or assembly 'Microsoft.UniversalStore.HardwareWorkflow.SubmissionBuilder, Version=1.0.6409.28204, Culture=neutral, PublicKeyToken=02dba2a134135739' or one of its dependencies. The system cannot find the file specified.

I have no clue. So let's just disable this if I haven't already: <!-- Would have a different answer if we're writing production code -->

### Via Visual Studio
Go to FsFltStrhide project properties. For the desired configuration/platform, navigate to 'Configuration Properties&nbsp;> Inf2Cat&nbsp;> General' and set 'Run Inf2Cat' to 'No'.

### Via manual file editing
In file FsFltStrhide.vcxproj, locate and modify the desired Configuration|Platform \<PropertyGroup\> as follows:
```xml
<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
  <!-- Add/Modify the below line -->
  <EnableInf2cat>false</EnableInf2cat>
</PropertyGroup>
```

if ( $PSVersionTable.PSVersion -lt "7.0.0" ) {
    Write-Warning "PowerShell 7 required; Install or upgrade your PowerShell version: https://aka.ms/powershell-release?tag=stable"  -ForegroundColor Yellow
    exit 1
}

if (!$env:QT_ROOT_DIR) {
    Write-Host "ERROR: `$env:QT_ROOT_DIR must be defined" -ForegroundColor Yellow
    Exit 1
}

if (-not (Test-Path "$env:VCINSTALLDIR")) {
    $VcBuildDir = $null
    $VsDirs = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community"
    )
    foreach ($VsDir in $VsDirs) {
        if (Test-Path -Path "$VsDir\VC\Auxiliary\Build\") {
          $VcBuildDir = "$VsDir\VC\Auxiliary\Build\"
          break
        }
    }
    if (!$VcBuildDir) {
        Write-Host "ERROR: Microsoft Visual Studio not found; tried:`r`n " ($VsDirs -join "`r`n  ") -ForegroundColor Yellow
        Exit 1    
    }
    "VcBuildDir=$VcBuildDir"

    Push-Location $VcBuildDir
    cmd /c "vcvars64.bat & set" | ForEach-Object {
        if ($_ -match "=") {
            $v = $_.split("=", 2)
            Set-Item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
        }
    }
    Pop-Location

    Write-Host "`nVisual Studio Command Prompt variables set.`n" -ForegroundColor Green
} else {
    Write-Host "`nVisual Studio Command Prompt variables already set.`n" -ForegroundColor Green
}

$SystemCoreCount = (Get-CimInstance -ClassName Win32_ComputerSystem).NumberOfLogicalProcessors
"SystemCoreCount=$SystemCoreCount"

$Qt6Dir = Resolve-Path -Path ${env:QT_ROOT_DIR}
"Qt6Dir=$Qt6Dir"
$QtVersion = (Get-Item $Qt6Dir\..).Basename.Replace(".", "_")
"QtVersion=$QtVersion"
$QtIfwVersion = (Get-ChildItem $Qt6Dir\..\..\Tools\QtInstallerFramework | Sort-Object -Property {$_.Name -as [int]} | Select-Object -Last 1).Basename
"QtIfwVersion=$QtIfwVersion"
$env:QtIfwVersion = $QtIfwVersion # Used in QtNdiMonitorCapture.pro to calc QT_IFW_DIR
$QtJomDir = Resolve-Path -Path $Qt6Dir\..\..\Tools\QtCreator\bin\jom
"QtJomDir=$QtJomDir"

$CheckoutDir = Resolve-Path -Path "$PSScriptRoot\.."
$AppName = (Get-Item $CheckoutDir).Basename
$BuildDir = "${CheckoutDir}\build-ci-${AppName}-Desktop_Qt_${QtVersion}_MSVC2022_64bit-Release"
"BuildDir=$BuildDir"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Push-Location $BuildDir

"qmake ..."
& $Qt6Dir\bin\qmake.exe ..\QtNdiProject.pro -spec win32-msvc $args
if ($LASTEXITCODE) {
    Pop-Location
    Exit 1
}

"jom qmake_all ..."
& $QtJomDir\jom.exe qmake_all
if ($LASTEXITCODE) {
    Pop-Location
    Exit 1
}

"jom ..."
& $QtJomDir\jom.exe /J $SystemCoreCount
if ($LASTEXITCODE) {
    Pop-Location
    Exit 1
}

Pop-Location

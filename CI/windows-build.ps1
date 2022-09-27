if (!${env:Qt6_DIR}) {
    Write-Host "ERROR: `$env:Qt6_DIR must be defined"
    return 1
}

if (-not (Test-Path env:VCINSTALLDIR)) {
    if (Test-Path -Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise") {
        $dir = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\"
    } else {
        $dir ="${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\"
    }

    pushd $dir
    cmd /c "vcvars64.bat & set" |
    foreach {
        if ($_ -match "=") {
            $v = $_.split("=", 2)
            Set-Item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
        }
    }
    popd

    Write-Host "`nVisual Studio Command Prompt variables set." -ForegroundColor Green
} else {
    Write-Host "`nVisual Studio Command Prompt variables already set." -ForegroundColor Yellow
}

$SystemCoreCount = (Get-CimInstance -ClassName Win32_ComputerSystem).NumberOfLogicalProcessors
"SystemCoreCount=$SystemCoreCount"

$Qt6_DIR = Resolve-Path -Path ${env:Qt6_DIR}
$QtVersion = (Get-Item $Qt6_DIR\..).Basename.Replace(".", "_")
$QtJom = Resolve-Path -Path $Qt6_DIR\..\..\Tools\QtCreator\bin\jom

$CheckoutDir = Resolve-Path -Path "$PSScriptRoot\.."
$AppName = (Get-Item $CheckoutDir).Basename
$BuildDir = "${CheckoutDir}\app\build-ci-${AppName}-Desktop_Qt_${QtVersion}_MSVC2019_64bit-Release"
"BuildDir=$BuildDir"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

pushd $BuildDir

& $Qt6_DIR\bin\qmake.exe ..\QtNdiMonitorCapture.pro -spec win32-msvc $args
& $QtJom\jom.exe qmake_all
& $QtJom\jom.exe /J $SystemCoreCount

popd

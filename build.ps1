param(
  [string]$Sdk = 'C:\ti\mspm0_sdk_2_06_00_05',
  [string]$Toolchain = 'C:\ti\ccs2101\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS'
)
$ErrorActionPreference = 'Stop'
$buildDir = Join-Path $PSScriptRoot 'build'
$compiler = Join-Path $Toolchain 'bin\tiarmclang.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw "Compiler missing: $compiler" }
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Copy-Item -Path (Join-Path $PSScriptRoot 'config\generated\*') -Destination $buildDir -Force
Start-Transcript -Path (Join-Path $buildDir 'build.log') -Force | Out-Null
try {
  $flags = @(
    '-O2', '-gdwarf-3', '-mcpu=cortex-m0plus', '-march=thumbv6m',
    '-mfloat-abi=soft', '-mthumb', '-D__MSPM0G3507__',
    '-isystem', (Join-Path $Sdk 'source'),
    '-I', (Join-Path $Sdk 'source\third_party\CMSIS\Core\Include'),
    '-I', (Join-Path $Sdk 'kernel\freertos\Source\include'),
    '-I', (Join-Path $Sdk 'kernel\freertos\Source\portable\TI_ARM_CLANG\ARM_CM0'),
    '-I', (Join-Path $PSScriptRoot 'config'),
    '-I', $buildDir
  )
  $kernelFiles = @(
    'kernel\freertos\Source\tasks.c',
    'kernel\freertos\Source\list.c',
    'kernel\freertos\Source\queue.c',
    'kernel\freertos\Source\portable\TI_ARM_CLANG\ARM_CM0\port.c',
    'kernel\freertos\Source\portable\TI_ARM_CLANG\ARM_CM0\portasm.c',
    'kernel\freertos\dpl\StaticAllocs_freertos.c',
    'kernel\freertos\dpl\AppHooks_freertos.c'
  ) | ForEach-Object { Join-Path $Sdk $_ }
  $kernelFiles += Join-Path $PSScriptRoot 'config\startup\startup_mspm0g350x_ticlang.c'
  $kernelFiles += Join-Path $buildDir 'ti_msp_dl_config.c'
  $objects = @()
  Write-Host 'Compiling FreeRTOS, startup and generated clock code...'
  foreach ($file in $kernelFiles) {
    $object = Join-Path $buildDir ([IO.Path]::GetFileNameWithoutExtension($file) + '.obj')
    & $compiler @flags -c $file -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $file" }
    $objects += $object
  }
  foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'src') -Filter '*.cpp' -Recurse)) {
    $object = Join-Path $buildDir ('app_' + $file.BaseName + '.obj')
    & $compiler @flags -std=c++17 -Wall -Wextra -Werror -c $file.FullName -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $file" }
    $objects += $object
  }
  $linkFlags = @('-mcpu=cortex-m0plus', '-march=thumbv6m', '-mfloat-abi=soft', '-mthumb', '-Wl,-u,_c_int00') + $objects + @(
    (Join-Path $buildDir 'device_linker.cmd'),
    (Join-Path $Sdk 'source\ti\driverlib\lib\ticlang\m0p\mspm0g1x0x_g3x0x\driverlib.a'),
    '-Wl,--rom_model', ('-Wl,-m,' + (Join-Path $buildDir 'reaction_game.map')),
    '-L', (Join-Path $Toolchain 'lib'), (Join-Path $Toolchain 'lib\libc.a'),
    '-o', (Join-Path $buildDir 'reaction_game.out'))
  & $compiler @linkFlags
  if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
  Write-Host 'PASS: build\reaction_game.out (not flashed)'
} finally { Stop-Transcript | Out-Null }

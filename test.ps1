param([string]$Compiler='C:\msys64\ucrt64\bin\g++.exe')
$ErrorActionPreference='Stop'
$build=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
& $Compiler -std=c++17 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'tests\host_tests.cpp') (Join-Path $PSScriptRoot 'src\display\oled.cpp') (Join-Path $PSScriptRoot 'src\display\software_i2c.cpp') -o (Join-Path $build 'host_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Host compilation failed' }
$savedPath=$env:PATH
try {
  $env:PATH=(Split-Path -Parent $Compiler)+';'+$savedPath
  & (Join-Path $build 'host_tests.exe')
  if ($LASTEXITCODE -ne 0) { throw 'Host tests failed' }
} finally { $env:PATH=$savedPath }

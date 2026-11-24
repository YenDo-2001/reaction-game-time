# Owner: Member 4. Explicitly program and verify the already-built firmware.
param([string]$DSLite='C:\ti\ccs2101\ccs\ccs_base\DebugServer\bin\DSLite.exe')
$ErrorActionPreference='Stop'
$image=Join-Path $PSScriptRoot 'build\reaction_game.out'
$targetConfig=Join-Path $PSScriptRoot 'targetConfigs\MSPM0G3507.ccxml'
foreach ($file in @($DSLite,$image,$targetConfig)) {
  if (!(Test-Path -LiteralPath $file)) { throw "Required file missing: $file" }
}
& $DSLite flash "--config=$targetConfig" --flash --verify --run --timeout=45 "--log=$(Join-Path $PSScriptRoot 'build\flash.log')" $image
if ($LASTEXITCODE -ne 0) { throw 'Programming or verification failed.' }
Write-Host 'PASS: image programmed, verified, and target instructed to run.'

param(
 [Parameter(Mandatory=$true)][string]$DevEco,
 [Parameter(Mandatory=$true)][string]$QtPrefix,
 [Parameter(Mandatory=$true)][string]$Moc,
 [Parameter(Mandatory=$true)][string]$Rcc,
 [Parameter(Mandatory=$true)][string]$Uic,
 [Parameter(Mandatory=$true)][string]$Output,
 [Parameter(Mandatory=$true)][string]$ToolHome,
 [string]$LocalSigningJson='',
 [ValidateSet('debug','release')][string]$BuildMode='release'
)
$ErrorActionPreference='Stop'
$cpu=(Get-Counter '\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 3).CounterSamples.CookedValue
Write-Output ('CPU_SAMPLES='+($cpu -join ','))
if(($cpu|Measure-Object -Maximum).Maximum -ge 70){throw 'RESOURCE_WAIT: no build started'}
$own=[Diagnostics.Process]::GetCurrentProcess();$own.ProcessorAffinity=3;$own.PriorityClass='BelowNormal'
$source=Split-Path -Parent $PSScriptRoot
foreach($p in @($Output,$ToolHome)){if([IO.Path]::GetFullPath($p).StartsWith('C:\',[StringComparison]::OrdinalIgnoreCase)){throw 'Use isolated non-C output and tool directories'}}
foreach($p in @('private-home','temp')){New-Item -ItemType Directory -Path (Join-Path $Output $p) -Force|Out-Null}
foreach($p in @('hvigor-home','ohpm-home','npm-cache')){New-Item -ItemType Directory -Path (Join-Path $ToolHome $p) -Force|Out-Null}
$env:USERPROFILE=Join-Path $Output 'private-home';$env:TEMP=Join-Path $Output 'temp';$env:TMP=$env:TEMP
[IO.File]::WriteAllText((Join-Path $env:USERPROFILE '.npmrc'),"registry=https://registry.npmjs.org/`n")
$env:HVIGOR_USER_HOME=Join-Path $ToolHome 'hvigor-home';$env:OHPM_HOME=Join-Path $ToolHome 'ohpm-home';$env:npm_config_cache=Join-Path $ToolHome 'npm-cache';$env:npm_config_userconfig=Join-Path $env:USERPROFILE '.npmrc'
$env:NODE_HOME=Join-Path $DevEco 'tools\node';$env:JAVA_HOME=Join-Path $DevEco 'jbr';$env:DEVECO_SDK_HOME=Join-Path $DevEco 'sdk';$env:OHOS_BASE_SDK_HOME=Join-Path $DevEco 'sdk\default\openharmony'
$env:PATH=$env:NODE_HOME+';'+$env:JAVA_HOME+'\bin;'+$env:PATH;$env:CMAKE_BUILD_PARALLEL_LEVEL='2'
$prepareArgs=@((Join-Path $PSScriptRoot 'prepare-windows.cjs'),$source,$Output,$QtPrefix,$Moc,$Rcc,$Uic)
if($LocalSigningJson){$prepareArgs+=$LocalSigningJson}
& (Join-Path $env:NODE_HOME 'node.exe') @prepareArgs
if($LASTEXITCODE -ne 0){throw 'Projection failed'}
Set-Location $Output
& (Join-Path $DevEco 'tools\ohpm\bin\ohpm.bat') install --all --registry https://ohpm.openharmony.cn/ohpm
if($LASTEXITCODE -ne 0){throw 'Dependencies failed'}
& (Join-Path $env:NODE_HOME 'node.exe') (Join-Path $DevEco 'tools\hvigor\bin\hvigorw.js') assembleApp --mode project -p product=default -p buildMode=$BuildMode --no-daemon --no-parallel --max-workers 2
exit $LASTEXITCODE

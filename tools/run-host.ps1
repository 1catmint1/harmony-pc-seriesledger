param([Parameter(Mandatory=$true)][string]$QtHost,[Parameter(Mandatory=$true)][string]$Compiler,[Parameter(Mandatory=$true)][string]$WorkDirectory,[Parameter(Mandatory=$true)][string]$PrivateDatabaseJson,[string]$FixtureDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cpu=(Get-Counter '\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 3).CounterSamples.CookedValue
if(($cpu|Measure-Object -Maximum).Maximum -ge 70){throw 'RESOURCE_WAIT'}
$own=[Diagnostics.Process]::GetCurrentProcess();$own.ProcessorAffinity=3;$own.PriorityClass='BelowNormal'
New-Item -ItemType Directory -Path $WorkDirectory -Force|Out-Null
$env:PATH=(Split-Path -Parent $Compiler)+';'+$QtHost+'\bin;'+$env:PATH
$moc=$WorkDirectory+'\moc_influxclient.cpp'
& ($QtHost+'\bin\moc.exe') ($root+'\app\include\influxclient.h') -o $moc
if($LASTEXITCODE -ne 0){throw 'moc failed'}
$includes=@(('-I'+$root+'\app\include'),('-I'+$QtHost+'\include'),('-I'+$QtHost+'\include\QtCore'),('-I'+$QtHost+'\include\QtGui'),('-I'+$QtHost+'\include\QtWidgets'),('-I'+$QtHost+'\include\QtNetwork'))
$sources=@(($root+'\tests\test-host.cpp'),($root+'\app\src\influxclient.cpp'),$moc)
& $Compiler -DSERIESLEDGER_CORE_ONLY -std=c++17 -O0 @includes @sources ('-L'+$QtHost+'\lib') -lQt5Network -lQt5Core -o ($WorkDirectory+'\seriesledger-host.exe')
if($LASTEXITCODE -ne 0){throw ('host compile failed: '+$LASTEXITCODE)}
if(!$FixtureDirectory){$FixtureDirectory=$root+'\tests\fixtures'}
& ($WorkDirectory+'\seriesledger-host.exe') ($FixtureDirectory+'\mixed-schema.json') ($FixtureDirectory+'\later-error.json') $PrivateDatabaseJson
exit $LASTEXITCODE

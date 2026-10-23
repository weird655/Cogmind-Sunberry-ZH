param([string]$GameDirectory,[ValidateSet('Menu','Install','Uninstall','Check')][string]$Mode='Menu',[string]$ResourceDirectory,[string]$InstallerPath,[string]$ResultFile)
$ErrorActionPreference='Stop'
if(!$GameDirectory){
 $candidate=[IO.DirectoryInfo]$PSScriptRoot
 for($depth=0;$candidate -and $depth -lt 6;$depth++){
  if([IO.File]::Exists((Join-Path $candidate.FullName 'COGMIND.exe'))){$GameDirectory=$candidate.FullName;break}
  $candidate=$candidate.Parent
 }
 if(!$GameDirectory){Write-Host '请把整个“cogmind汉化261019”文件夹放到 COGMIND.exe 旁边，再打开安装。' -ForegroundColor Yellow;exit 1}
}
if(!$ResourceDirectory){$ResourceDirectory=$PSScriptRoot}
if($Mode -eq 'Menu'){
 Write-Host '=== cogmind汉化261019 ===' -ForegroundColor Green
 Write-Host '回车：安装汉化    2：检查安装    3：卸载汉化    0：退出' -ForegroundColor Green
 do{$choice=Read-Host '请选择'}while($choice -notin @('','1','2','3','0'))
 if($choice -eq '0'){exit 0}
 $Mode=if($choice -eq '2'){'Check'}elseif($choice -eq '3'){'Uninstall'}else{'Install'}
}
Write-Host '=== Cogmind 汉化 0.2.2 · Steam Beta 17.1（260906a） ===' -ForegroundColor Green
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Management/Microsoft.PowerShell.Management.psd1') -ErrorAction Stop
$owner='CogmindSunberryZH-010-v1'
$expectedExe='6d24c525c12de38f3fc21017b023f22ea6d3ec94e3b160d83ef0570327701666'
$expectedP='fd1f00effa3ed69c8e8286cad9566183a6a54d803cc5040c51b97ce3defb5427'
$expectedM='b420f2daf871e19b5e4819dce8d0061098f61693d7457ff7117a492d44a47900'
$expectedPhysfs='6cae6aa7d84a562dd6735304c7b5a195dd930ba233f174cb71b6d929a5c39ebc'
$game=[IO.Path]::GetFullPath($GameDirectory).TrimEnd('\','/')
$zhDirectory=Join-Path $game 'CogmindZH'
$statePath=Join-Path $zhDirectory 'install-state.json'
$exe=Join-Path $game 'COGMIND.exe'
$stage=$null
function Hash([string]$path){
 if(!(Test-Path -LiteralPath $path -PathType Leaf)){return ''}
 return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function AtomicCopy([string]$source,[string]$destination){
 $temp=$destination+'.zh-tmp-'+[Guid]::NewGuid().ToString('N')
 try {
  [IO.File]::Copy($source,$temp,$false)
  if((Hash $source) -ne (Hash $temp)){throw '复制校验失败'}
  if([IO.File]::Exists($destination)){[IO.File]::Replace($temp,$destination,[NullString]::Value)}
  else{[IO.File]::Move($temp,$destination)}
 } finally {if([IO.File]::Exists($temp)){[IO.File]::Delete($temp)}}
}
function SaveState($value){
 $temp=$statePath+'.new'
 [IO.File]::WriteAllText($temp,($value|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($true))
 if([IO.File]::Exists($statePath)){[IO.File]::Replace($temp,$statePath,[NullString]::Value)}else{[IO.File]::Move($temp,$statePath)}
}
function AssertStopped {
 if(Get-Process COGMIND -ErrorAction SilentlyContinue | Where-Object {$_.Path -and $_.Path.Equals($exe,[StringComparison]::OrdinalIgnoreCase)}){throw '请先正常退出此目录的 Cogmind，再安装或卸载。'}
}
function Restore($state){
 if($state.owner -ne $owner){throw '安装记录不属于本汉化，拒绝覆盖。'}
 $backup=Join-Path $zhDirectory 'backups/physfs.original.dll'
 if((Hash $backup) -ne $expectedPhysfs){throw '原版 DLL 备份损坏，拒绝恢复。'}
 $current=Hash (Join-Path $game 'physfs.dll')
 if($current -ne $state.proxyHash -and $current -ne $expectedPhysfs){throw 'physfs.dll 已被其他程序修改，拒绝覆盖；备份仍保留。'}
 $legacy=Join-Path $game 'WININET.dll'
 if($state.legacyHash -and (Hash $legacy) -and (Hash $legacy) -ne $state.legacyHash){throw 'WININET.dll 已被其他插件替换，拒绝自动覆盖。'}
 # Restore engine import first, so later cleanup cannot leave an active proxy.
 AtomicCopy $backup (Join-Path $game 'physfs.dll')
 if($state.legacyHash){
  $old=Join-Path $zhDirectory 'backups/WININET.before-install.dll'
  if((Hash $old) -ne $state.legacyHash){throw '旧插件备份损坏；原版 physfs 已恢复，备份目录保留。'}
  AtomicCopy $old $legacy
 }
 $language=Join-Path $game 'native-language.txt'
 if((Hash $language) -eq $state.languageHash){
  if($state.languageBefore){AtomicCopy (Join-Path $zhDirectory 'backups/language.before-install') $language}
  else{[IO.File]::Delete($language)}
 }
 $alias=Join-Path $game 'physfs_zh_original.dll'
 if(!$state.aliasBefore -and (Hash $alias) -eq $expectedPhysfs){[IO.File]::Delete($alias)}
 foreach($file in $state.files){
  $path=Join-Path $game $file.relative
  if(!$file.before -and (Hash $path) -eq $file.sha256){[IO.File]::Delete($path)}
 }
 # Keep the original backup and running uninstaller as a recoverable checkpoint.
 if([IO.File]::Exists($statePath)){
  $last=Join-Path $zhDirectory 'last-uninstall-state.json'
  [IO.File]::Copy($statePath,$last,$true);[IO.File]::Delete($statePath)
 }
}
try {
 if(!(Test-Path -LiteralPath $exe -PathType Leaf)){throw '此文件夹没有 COGMIND.exe。'}
 AssertStopped
 if($Mode -ne 'Uninstall' -and (Test-Path -LiteralPath (Join-Path $game 'WININET.dll'))){throw 'Other injection DLL detected. Uninstall the old loader first.'}
 if($Mode -eq 'Uninstall'){
  if(!(Test-Path -LiteralPath $statePath)){throw '此目录没有本安装器的安装记录，未改动任何文件。'}
  $state=Get-Content -LiteralPath $statePath -Raw -Encoding UTF8|ConvertFrom-Json
  Restore $state
  $message='已卸载本次汉化，并恢复安装前的 DLL 与旧插件。备份和卸载器保留在 CogmindZH 文件夹，可自行保留或删除。'
 } else {
  if((Hash $exe) -ne $expectedExe -or (Hash (Join-Path $game 'data/particles.bin')) -ne $expectedP -or (Hash (Join-Path $game 'data/messages.bin')) -ne $expectedM){throw '此包仅支持已验证的 Steam Beta 17.1（260906a）程序及原版数据库。文件指纹不符，未安装。请勿替换游戏 EXE 绕过校验。'}
  $stage=[IO.Path]::GetFullPath($ResourceDirectory)
  $manifest=Get-Content -LiteralPath (Join-Path $stage 'manifest.json') -Raw -Encoding UTF8|ConvertFrom-Json
  $allowed=@('autoload/physfs.dll','CogmindZH/bin/native_zh.dll','CogmindZH/zh_para.tsv','CogmindZH/item_names.tsv','CogmindZH/zh_trans.tsv','CogmindZH/zh_extra.tsv','CogmindZH/runtime_patches.tsv','CogmindZH/VERSION')
  if(@($manifest.files).Count -ne $allowed.Count -or @($manifest.files.relative|Select-Object -Unique).Count -ne $allowed.Count){throw 'Unexpected payload file set.'}
  foreach($file in $manifest.files){if($allowed -notcontains $file.relative){throw 'Unexpected payload path.'};if((Hash (Join-Path $stage $file.relative)) -ne $file.sha256){throw ('安装资源校验失败：'+$file.relative)}}
  $proxyHash=Hash (Join-Path $stage 'autoload/physfs.dll')
  if(Test-Path -LiteralPath $statePath){
   $state=Get-Content -LiteralPath $statePath -Raw -Encoding UTF8|ConvertFrom-Json
   if($state.owner -ne $owner -or (Hash (Join-Path $game 'physfs.dll')) -ne $state.proxyHash){throw '安装状态冲突，拒绝覆盖。请先检查或卸载旧版本。'}
   foreach($file in $state.files){if((Hash (Join-Path $game $file.relative)) -ne $file.sha256){throw '现有汉化内容已改动，请先备份；安装器不覆盖自定义译表。'}}
   if($state.proxyHash -ne $proxyHash){throw '请先卸载现有自动加载版，再安装此版本。'}
   $message='本版本已安装且校验完整。直接启动原版 COGMIND.exe 或从 Steam 启动即可加载中文。'
  } else {
   if((Hash (Join-Path $game 'physfs.dll')) -ne $expectedPhysfs){throw 'physfs.dll 不是受支持的原版 DLL，可能已有其他 Mod Loader。未覆盖它。'}
   $aliasBefore=Test-Path -LiteralPath (Join-Path $game 'physfs_zh_original.dll')
   if($aliasBefore -and (Hash (Join-Path $game 'physfs_zh_original.dll')) -ne $expectedPhysfs){throw '原 DLL 转发文件名已被其他文件占用。'}
   $files=@()
   foreach($file in $manifest.files){
    if(!$file.relative.StartsWith('CogmindZH/')){continue}
    $target=Join-Path $game $file.relative;$before=Test-Path -LiteralPath $target
    if($before -and (Hash $target) -ne $file.sha256){throw ('已有不同文件，拒绝覆盖：'+$file.relative)}
    $files+=@{relative=$file.relative;sha256=$file.sha256;before=[bool]$before}
   }
   if($Mode -eq 'Check'){$message='安装前检查通过；未替换 DLL、未复制存档、未启动游戏。'}
   else {
    New-Item -ItemType Directory -Force -Path (Join-Path $zhDirectory 'backups')|Out-Null
    if(Test-Path -LiteralPath (Join-Path $game 'user')){Copy-Item -LiteralPath (Join-Path $game 'user') -Destination (Join-Path $zhDirectory 'backups/user-before-install') -Recurse}
    $backup=Join-Path $zhDirectory 'backups/physfs.original.dll'
    if((Test-Path -LiteralPath $backup) -and (Hash $backup) -ne $expectedPhysfs){throw '已有不同的原版备份，拒绝覆盖。'}
    AtomicCopy (Join-Path $game 'physfs.dll') $backup
    $legacyHash=Hash (Join-Path $game 'WININET.dll')
    if($legacyHash){
     $old=Join-Path $zhDirectory 'backups/WININET.before-install.dll'
     if((Test-Path -LiteralPath $old) -and (Hash $old) -ne $legacyHash){throw '已有不同的旧插件备份，拒绝覆盖。'}
     AtomicCopy (Join-Path $game 'WININET.dll') $old
    }
    $languageBefore=Test-Path -LiteralPath (Join-Path $game 'native-language.txt')
    if($languageBefore){AtomicCopy (Join-Path $game 'native-language.txt') (Join-Path $zhDirectory 'backups/language.before-install')}
    $state=@{owner=$owner;version=$manifest.version;proxyHash=$proxyHash;aliasBefore=[bool]$aliasBefore;files=$files;legacyHash=$legacyHash;languageBefore=[bool]$languageBefore;languageHash='';complete=$false}
    SaveState $state
    try {
     foreach($file in $files){
      $target=Join-Path $game $file.relative
      New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target)|Out-Null
      AtomicCopy (Join-Path $stage $file.relative) $target
     }
     AtomicCopy $backup (Join-Path $game 'physfs_zh_original.dll')
     if($legacyHash){[IO.File]::Delete((Join-Path $game 'WININET.dll'))}
     [IO.File]::WriteAllText((Join-Path $game 'native-language.txt'),'zh',[Text.Encoding]::ASCII)
     $state.languageHash=Hash (Join-Path $game 'native-language.txt');SaveState $state
     if($InstallerPath){AtomicCopy $InstallerPath (Join-Path $zhDirectory '卸载汉化.cmd')}
     AtomicCopy (Join-Path $stage 'autoload/physfs.dll') (Join-Path $game 'physfs.dll')
     $state.complete=$true;SaveState $state
    } catch { $errorText=$_.Exception.Message;try{Restore $state}catch{throw ($errorText+'；恢复未完成：'+$_.Exception.Message)};throw ($errorText+'；已恢复安装前 DLL。') }
    $message='汉化安装成功！以后直接启动原版 COGMIND.exe，或在 Steam 中点“开始游戏”，即可加载中文。右 Alt＋F9 切换中英文。要卸载时，再打开安装汉化.cmd，输入 3。'
   }
  }
 }
 Write-Host $message -ForegroundColor Green
 if($ResultFile){[IO.File]::WriteAllText($ResultFile,$message,[Text.Encoding]::Unicode)}
 exit 0
} catch {
 Write-Host $_.Exception.Message -ForegroundColor Red
 if($ResultFile){[IO.File]::WriteAllText($ResultFile,$_.Exception.Message,[Text.Encoding]::Unicode)}
 exit 1
}

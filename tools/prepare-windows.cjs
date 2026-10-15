const fs=require('fs'),path=require('path'),crypto=require('crypto');
const [sourceArg,destArg,sdkArg,mocArg,rccArg,uicArg,signingArg]=process.argv.slice(2);
if(!sourceArg||!destArg||!sdkArg||!mocArg||!rccArg||!uicArg)throw Error('Usage: prepare-windows source candidate QtTargetRoot moc.exe rcc.exe uic.exe [private-signing.json]');
const source=path.resolve(sourceArg),dest=path.resolve(destArg),sdk=path.resolve(sdkArg);
if(dest===source || dest.startsWith(source+path.sep))throw Error('Candidate must be outside source');
const json5=require(process.env.SERIESLEDGER_JSON5 || 'json5');fs.mkdirSync(dest,{recursive:true});
for(const rel of ['AppScope','app','entry/src','entry/build-profile.json5','entry/hvigorfile.ts','entry/oh-package.json5','entry/obfuscation-rules.txt','hvigor','hvigorfile.ts','oh-package.json5','build-profile.json5'])fs.cpSync(path.join(source,rel),path.join(dest,rel),{recursive:true});
const update=(rel,fn)=>{const p=path.join(dest,rel),o=json5.parse(fs.readFileSync(p,'utf8'));fn(o);fs.writeFileSync(p,JSON.stringify(o,null,2)+'\n');};
const slash=p=>path.resolve(p).replace(/\\/g,'/');
update('build-profile.json5',o=>{delete o.app.signingConfigs;for(const p of o.app.products){delete p.signingConfig;p.compatibleSdkVersion='6.0.2(22)';p.targetSdkVersion='6.0.2(22)';}if(signingArg){o.app.signingConfigs=json5.parse(fs.readFileSync(signingArg,'utf8'));for(const p of o.app.products)p.signingConfig=o.app.signingConfigs[0].name;}});
for(const rel of ['oh-package.json5','hvigor/hvigor-config.json5'])update(rel,o=>o.modelVersion='5.1.0');
update('entry/build-profile.json5',o=>o.buildOption.externalNativeOptions.arguments='-DQT_PREFIX='+slash(sdk)+' -DQT_HOST_MOC='+slash(mocArg)+' -DQT_HOST_RCC='+slash(rccArg)+' -DQT_HOST_UIC='+slash(uicArg));
const receipt=JSON.parse(fs.readFileSync(process.env.SERIESLEDGER_QT_RECEIPT || path.join(source,'evidence/target-qt-library-byte-binding.json'),'utf8').replace(/^\uFEFF/,''));
for(const expected of receipt){const data=fs.readFileSync(path.join(sdk,'lib',expected.name));const digest=crypto.createHash('sha1').update('blob '+data.length+'\0').update(data).digest('hex');if(digest!==expected.upstreamBlob)throw Error('Target Qt library does not match evidence: '+expected.name);}
console.log('Separate API22 candidate prepared with SHA-bound Qt5.15.12; personal signing remains private.');

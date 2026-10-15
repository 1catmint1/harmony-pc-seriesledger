# SeriesLedger · InfluxDB Studio 查询结果版

基于 CymaticLabs InfluxDB Studio 与社区 Qt/HarmonyOS 适配，修复多条语句和不同 series 结果的错列、tag 丢失、后项错误误判成功，以及查询输入/页面生命周期变化后的旧回调绑定。本轮源自固定上游 https://atomgit.com/OpenHarmonyPCDeveloper/ohos_InfluxDBStudio ，提交489fcd58006d73d31b687c007a2ffa82869d9547。

原项目是 .NET/WinForms，ohos目录已采用 Qt Widgets/Network 与 ArkTS QPA 重写。本版继续该社区路径，不宣称原 .NET 运行时、InfluxData.Net 或所有 WinForms 功能完整移植。当前发行候选1.0.2使用独立SeriesLedger品牌，包名com.seriesledger.track1。发行说明和隐私见 RELEASE.md 与 PRIVACY.md。

## 改进

- 直接逐值百分号编码 GET/POST 的数据库及 SQL，避免未转义分号导致真实 InfluxDB HTTP400，也保留百分号序列、加号、&和中文。
- 检查全部 results.error；任一语句失败都显示 statement_id，清除旧表格和旧JSON导出document，避免部分成功被误认为完整成功。
- 按 statement_id、measurement、tag.*、field.* 对齐统一表格，不沿用第一组列名，缺失字段留空；保留 tag 来源并避免字段名与元数据同名冲突。
- 校验响应结构，限制8MiB响应、15秒请求、256列及10000行；丢弃输入/数据库/revision变化和页面关闭后的旧回调，使用QPointer处理客户端生命周期。
- 关闭查询标签释放页面；CSV对CR同样引用。既有运维菜单、设置及Qt/ArkTS启动链路保留原归属。

## 实际验证

Windows Qt5.15.19：23/23结果控制、6/6真正Qt客户端连接官方InfluxDB1.13.1。真实USB API23 PC、Qt5.15.12：23/23结果控制、13/13实库与Widget控制、skip0。控制覆盖实库多语句、多series、多schema、epoch ms、后项error、401、POST保真及旧输入/数据库/关闭页面、客户端销毁、表格恢复状态。

实际GUI按钮：预载临时验收会话，点击生产“执行查询”显示3行CPU/memory数据，8列按语义对齐；预载后项错误语句并点击生产按钮，显示statement1 database-not-found且旧表格清空。预载会话和错误预置属于私有测试instrumentation，不是发布版本内置数据库/密码或功能按钮。参考 evidence 下两张截图及验证JSON。凭据只在私有验收内存/本地配置，未保存到应用连接设置或公开代码。

服务器为官方Windows1.13.1包，SHA见 evidence/database-package.json；loopback认证实例经USB HDC rport连接，未用兼容模拟服务替代。目标Core/Gui/Network/Widgets/OhExtras五库按Git blob header+bytes逐一匹配固定上游，见 evidence/target-qt-library-byte-binding.json。

R1目标合成“同名字段”控制失败，未计PASS；改为显式QString/append构造JSON后R2通过。尚未把初始化差异单独归因于Qt版本或编译器。早期Windows完整Widgets测试程序因缺图形运行库0xC0000135未执行，主机只验Core/Network；Widget控制在目标运行。所有失败与警告保留本地证据，未宣称CI、上架批准或正式竞赛提交。

## 构建与复现

本仓库包含业务源码与ArkTS宿主，不包含Qt SDK、so、debug HAP、签名或测试密码。上游SDK可从上述固定提交单独取得。保留源码元数据API17/model6；Windows复现脚本生成单独API22/model5.1候选，公开包org.seriesledger.pc，个人调试配置自行提供。

使用Node和json5（可设置SERIESLEDGER_JSON5为DevEco SDK现有json5模块路径），运行 tools/prepare-windows.cjs，参数为源码目录、源码外候选目录、目标Qt5.15.12 SDK、三个Windows Qt5 moc/rcc/uic路径，可选私有签名JSON。路径建议无空格。脚本核验五库SHA绑定并移除源签名。使用DevEco SDK22、JBR与Node，ohpm install --all后执行assembleHap --mode module -p product=default -p module=entry@default -p buildMode=debug --no-daemon --no-parallel --max-workers 2；CMAKE_BUILD_PARALLEL_LEVEL=2。签名配置、用户目录、缓存、临时目录分别隔离，密钥留私有D盘；新的org包不得套用com调试Profile。

公开org.seriesledger.pc投影已完整构建unsigned HAP，SHA见evidence/public-build.json，但未在目标运行；不能把私有com.seriesledger.track1 R2执行说成org包已执行。数据库/查询测试可用 tools/run-host.ps1，传入QtHost、Compiler、F盘WorkDirectory和私有数据库JSON。测试JSON字段database/username/password；专用loopback18086实例，数据库track1_influx_validation，按tests/fixtures/data.lineprotocol写入fixture，不要指向生产数据库。原始响应fixtures与真实包校验可供只读核对。

## 范围与许可

原CymaticLabs代码MIT，见LICENSE-CYMATIC.txt；入口包既有元数据声明GPL-3.0-or-later，见LICENSE-ENTRY.txt的GPLv3文本，保持该声明；Qt及其它组件按各自原条款，未统一重写为MIT。PORTING_NOTES保留社区启动链路出处，原C#源码仍在上游src目录。

未验收：完整运维/用户授权菜单、TLS与证书例外、Picker文件导出、长期查询及超时/超限实际触发、全部原版功能。Qt5 QJson数值通道不保证大于2^53整数精确，17位仅保证已解析double的roundtrip。1.0.1保存和导出连接配置不写入password字段；旧导入配置可将密码读入当前内存会话，原文件由用户自行管理。生产序列化头文件新增3项主机控制；未宣称完成目标实际文件内容检查。没有正式导入/认领任务；空负责人和公开检索无结果均不证明无人负责。Apps提审、管理员上架与竞赛正式提交是不同状态。

## 1.0.1发行补充

本次新主机23核心/6真实数据库/3密码序列化控制。新真机R1是生产界面手动连接、查询3行、后项错误清表及隐私入口，共3项GUI结果，无包内测试注入；上述历史23/13原生控制不自动计入新轮。发行HAP全部23项ZIP运行内容与USB验证包相同；正式上传、版本提审与上架分别记录。Qt5.15.12对应源码和补丁闭包仍待核对。

Windows可使用tools/build-windows.ps1，BuildMode默认为release，生成源码外API22项目并assembleApp；Qt5目标五库SHA保持绑定，所有密钥只从私有LocalSigningJson读入。tools/run-host.ps1可通过FixtureDirectory明确指定已展开的fixture目录；编译器目录会加入当前进程PATH。


## 1.0.2窗口恢复修复
1.0.1云自检27项中26通过，窗口恢复项4次崩溃，失败证据保留。无子窗口标识的Want稳定复用主窗口，移除对当前QPA未提供getter的依赖；保留Qt子窗口标识，不记录完整Want参数。生产AbilityStage主机10控制（含30次重复主窗口Want）通过；USB R2的6次最大化-最小化-重新唤起均保持同一进程/UID/boot及原最大化边界，隐私子窗口可读，进程已释放。正式HAP的23项ZIP运行内容与R2调试签名发行模式包完全相同，APP内HAP运行内容相同；新云自检、提审及公开上架尚待。旧数据库和核心控制未重跑，不能并入新分母。

主机路由控制：设置SERIESLEDGER_TYPESCRIPT为本机typescript模块路径后运行node tools/test-ability-routing.cjs。此控制执行生产AbilityStage源码，QPA仅提供attachAbilityStage，不伪造缺失的getter；它不替代真实窗口恢复验收。

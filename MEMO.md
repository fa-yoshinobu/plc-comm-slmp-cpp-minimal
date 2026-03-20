# SLMP 実機検証・互換性データベース (2026-03-19)

## 1. 検証環境（クライアント側）
- **C++実装**: W6300-EVB-Pico2 (RP2350) + Arduino-Pico
- **Python実装**: Windows PC (Reference Implementation)

## 2. PLC機種別・コマンド対応マトリクス
各機種の「内蔵ポート」における SLMP コマンドの真の対応状況です。

| 命令コード | 命令名 | **iQ-R (R120P)** | Q (内蔵) | L26 (内蔵) | iQ-L (L16H) | QJ71E71 | 備考 |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **`0101`** | 型式読出し | **○** | × | × | ○ | ○ | iQ系/L/Q外部なら可。 |
| **`0401/1401`** | 一括読出し/書込み | **○** | ○ | ○ | ○ | ○ | 全機種共通。 |
| **`0403/1402`** | ランダムR/W | **○** | ○ | ○ | ○ | ○ | 全機種共通。 |
| **`0801/0802`** | モニタ登録/実行 | **△** | ○ | ○ | ○ | ○ | iQ-R/Lは8B形式で0xC061。 |
| **`0406/1406`** | ブロック一括R/W | **○** | × | × | ○ | ○ | iQ系/L/Q外部なら可。 |
| **`1001/1002`** | リモート操作 | **○** | × | × | ○ | ○ | iQ系/L/Q外部なら可。 |
| **`0613/1613`** | 自局メモリR/W | **○** | × | × | ○ | ○ | iQ系/L/Q外部なら可。 |

## 3. 実機検証から得られた知見
### iQ-R シリーズ (R120PCPU) の特性
- **SLMP の真のフルスペック**: 内蔵ポートでありながら、すべての拡張・管理コマンドを完璧にサポートしている。
- **Modern 4E 通信**: 高速・大容量通信に最適化されており、iQ-R を使用する場合は `Modern (iQR)` モードでの運用が強く推奨される。

### iQ-L (L16H) との共通点
- L16H も iQ-R と同等の「iQ 世代エンジン」を積んでいることが、コマンド対応状況の一致から再確認された。

### 接続の鉄則 (2026-03-19 改訂版)
1. **iQ-R / iQ-F / iQ-L**: `Modern (iQR)` モード対応。フル機能利用可能。
2. **Q / L (標準) シリーズ**: `Legacy (QL)` モード + `3E` フレーム。
   - 内蔵ポート使用時は、基本R/Wとモニタ機能に絞って設計する。
   - フル機能が必要な場合は、外部ユニット(QJ71等)を使用する。

## 4. 修正履歴と実装上の決定事項 (2026-03-20)

### CIエラーの分析と対策詳細

2026-03-20のCI実行（Static Analysis / pio check）で報告された問題の具体的箇所と対策案です。

#### ① プロトコルバグ: ブロック一括R/W (0406/1406) のサブコマンド
- **詳細**: 現状 iQ-R モードで `0x0002` を使用しているが、仕様上は `0x0000` 固定が正しい。
- **具体的修正箇所**: 
  - **ファイル**: `src/slmp_minimal.cpp`
  - **関数**: `SlmpClient::startAsync` (L595付近)
  - **内容**: `case AsyncContext::Type::ReadBlock` および `WriteBlock` の `subcommand` を `0x0000` に固定する。
- **テストの更新**: 
  - `tests/slmp_minimal_tests.cpp` 内の `makeGenericRequest(0x1406, 0x0002)` 等を `0x0000` に修正。
  - `tests/python_golden_frames.h` の `kReadBlockRequest` 18バイト目を `0x00` に修正。

#### ② 静的解析: 変数名 `index` の Shadowing (Style Error)
- **詳細**: グローバル関数 `index()` との衝突警告。
- **具体的修正箇所**: 
  - `src/slmp_minimal.cpp:1151`: `writeBits` 内の `index` -> `bit_index`
  - `examples/shared/slmp_wifi_serial_console.h`: コマンドパース関数内の `index` -> `token_index`
  - `examples/atom_matrix_serial_console/atom_matrix_serial_console.h:563`: `copyText` 内の `index` -> `i`

#### ③ 環境依存: W6300用ヘッダーの `#error` 停止
- **具体的修正箇所**: `examples/w6300_evb_pico2_serial_console/w6300_evb_pico2_serial_console.h:8`
- **内容**: `#else #error` を `#elif !defined(CPPCHECK) #error` に変更し、静的解析時のみパスさせる。

#### ④ コードの健全性（Style/Performance）に関する指摘
CIログより抽出された、改善が推奨される箇所です。

- **const化の推奨 (constVariable / constParameter)**:
  - `atom_matrix_serial_console.h`: `zeros` (L576, 584, 602, 614), `zero_words` (L627), `zero_dwords` (L628), `zero_bits` (L636) を `const array` に。
  - `slmp_wifi_serial_console.h`: 各種パース関数の `tokens` (L1101) や `points_token` / `value_token` (L899, 1127等) を `const` ポインタに。
- **到達不能・常に真偽が固定の条件 (knownConditionTrueFalse)**:
  - `slmp_wifi_serial_console.h`: `copy_length < token_length` (L321), `value_count <= 0` (L939, 971, 1222, 1283)
  - `atom_matrix_serial_console.h`: `value_count <= 0` (L1687)
- **変数のスコープ縮小 (variableScope)**:
  - `slmp_wifi_serial_console.h`: `request_hex` (L653), `response_hex` (L654)
#### ⑤ コンソールメニューの整理 (W6300-EVB-Pico2)
- **内容**: 一般利用者向けではない「低レベルな動作確認コマンド」をメニューおよび実装から除外。
- **除外したコマンド**: `FUNCHECK`, `FULLSCAN`, `ENDURANCE`, `RECONNECT`, `TXLIMIT`, `BENCH`, `DUMP`, `TARGET`, `MONITOR`
- **目的**: コンソール表示の簡素化と、ソースコードの可読性向上。


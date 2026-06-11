# refactor-instructions.md

plc-comm-slmp-cpp-minimal のリファクタリング指示書。
この文書は実装担当モデル向けの完結した作業指示である。実装前にこの文書全体を読むこと。

> **最重要の前提**: このライブラリは PlatformIO Registry に公開済み
> (`slmp-connect-cpp-minimal` 0.4.10)の組込み向け C++ ライブラリであり、
> README の Design Philosophy に「コア低レベル層 = 固定バッファ・ヒープ割当なし、
> 任意の高レベル層」という**意図的な 2 層・少ファイル構成**が明文化されている。
> さらに CI(`ci.yml`)はソースファイル名を**明示列挙**して g++ でビルドしている
> (sanitizer / coverage の 3 ジョブとも)。
> **したがって `src/` のファイル分割・追加・rename は本タスクでは禁止する**
> (やりたければ提案として報告のみ)。
>
> テスト体制は一族で最も厚い(ホストテスト + ASan/UBSan + カバレッジ +
> ソケット統合 + GX Simulator3 検証 + マトリクス検証)。**構造は健全であり、
> 本タスクで必須の変更は無い可能性が高い。**変更すべきものが見つからなければ、
> それを正直に報告して終了してよい。無理に変更量を増やすことを最も強く禁ずる。

---

## Objective

公開ヘッダ・ワイヤバイト列・割当なし保証・ファイル構成を一切壊さずに:

1. **ファイル内の限定的な内部整理に限る小さな改善**を、テストで挙動固定しながら行う
   (下記 Debt Map の D1 のみ。それ以外は提案として報告)

---

## Project Understanding

### 何のライブラリか

三菱 MELSEC PLC と SLMP バイナリ 3E/4E で通信する、マイコン向け C++ ライブラリ。
ESP32 / Arduino 系 / ホスト(POSIX)対応。コア(`slmp_minimal.h/.cpp`)は
呼び出し側所有バッファ・ヒープ割当なし。高レベル層(`slmp_high_level.h/.cpp`)は
文字列アドレス(`D100` / `D200:F` / `D50.3`)・型付きヘルパ・ポーリングプラン。
Python / .NET 版と高レベルの意味的互換。

### ファイル構成(src/、計約 8,500 行)

| ファイル | 行数 | 内容 |
|---|---|---|
| `slmp_minimal.cpp` / `.h` | 3,286 / 1,278 | コア(フレーム組立・パース・sync/async リクエストフロー)— **単一 TU は意図的** |
| `slmp_high_level.cpp` / `.h` | 1,667 / 665 | 高レベル層 |
| `slmp_error_codes.cpp` ほか | 1,454 | エンドコード表(言語別メッセージ分離済み) |
| `slmp_arduino_transport.h` / `slmp_utility.h` | 363 | トランスポート・ユーティリティ |

### テスト / CI / 検証コマンド

- `tests/slmp_minimal_tests.cpp`(2,051 行)+ ソケット統合(`scripts/
  run_socket_integration.py` + `scripts/mock_plc_server.py`)+ GX Simulator3 /
  マトリクス検証(実機系。実行しない)
- CI: ホストテスト(g++ 明示列挙)→ ASan/UBSan → カバレッジ → PIO 4 env ビルド
- ローカル: `run_ci.bat`(PIO 2 env ビルド)、`run_static_analysis.bat`

---

## Behaviors To Preserve(絶対に壊さない既存挙動)

1. **公開ヘッダの API 全て**(`slmp_minimal.h` / `slmp_high_level.h` /
   `slmp_error_codes.h` / `slmp_arduino_transport.h` / `slmp_utility.h`)。
2. **送信フレームのバイト列**(テストが契約)。
3. **コア層のヒープ割当なし保証**(README の設計哲学。new/malloc/std::string 等を
   コアパスに持ち込まない)。
4. **`SLMP_ENABLE_UDP_TRANSPORT` 等のビルドフラグの意味**(TODO.md にバイナリサイズ
   実測記録あり)。
5. **ファイル構成**: `src/` のファイル名一覧(CI の明示列挙・PIO パッケージングが依存)。
6. **バージョン 0.4.10・library.json / library.properties・CHANGELOG**: 変更しない。

---

## Non-Negotiables(交渉不可の制約)

- 最初に `git status` を確認する。未コミット変更があれば混ぜず、報告して停止する。
- 編集前に Baseline Commands をすべて実行し、結果(テスト件数含む)を記録する。
- `src/` のファイル追加・削除・rename をしない。
- 公開ヘッダの宣言を変更しない(実装ファイル内の整理に限る)。
- 依存を追加しない。`platformio.ini` / `library.json` / CI を変更しない。
- 既存テストの既存アサーションを変更しない(追加のみ可)。
- 実機 PLC への接続・GX Simulator3 を使う検証を行わない。
- 正しさが不明な場合は実装を止め、「Stop And Ask」として質問を報告書に書く。

---

## Stop And Ask Conditions(即時停止して質問する条件)

- 既存テスト・サニタイザ・ソケット統合が自分の変更後に落ちた ⇒ 即座に巻き戻して報告
- 公開ヘッダ・フレームバイト列・割当なし保証に影響しうる変更が必要に見えた
- D1 の整理対象が想定外に広範囲(複数関数・公開挙動)に波及した
- 本書に無い大きな問題を発見した(報告のみ)

---

## Baseline Commands

作業ディレクトリ: リポジトリルート。ホストに g++(または同等)と Python 3 が必要。
実機 PLC 不要・接続禁止。

```bash
git status
# CI と同一のホストテスト(Windows では WSL / MinGW g++ 等。無ければ未実施と報告)
g++ -std=c++17 -Wall -Wextra -Isrc tests/slmp_minimal_tests.cpp src/slmp_minimal.cpp src/slmp_error_codes.cpp src/slmp_error_messages.cpp src/slmp_error_messages_en.cpp src/slmp_error_messages_ja.cpp src/slmp_high_level.cpp -o /tmp/slmp_minimal_tests
/tmp/slmp_minimal_tests
python scripts/run_socket_integration.py --compiler g++
```

PlatformIO がある場合は追加で(`run_ci.bat` 相当):

```bash
pio run -e esp32-devkitc-low-level -e esp32-devkitc-high-level
```

---

## Debt Map

### D1. ファイル内の重複ヘルパ・長大関数 【調査の上、限定的に実装可】

- **根拠**: `slmp_minimal.cpp`(3,286 行)/ `slmp_high_level.cpp`(1,667 行)は
  単一 TU 方針のため、内部に類似のエンコード/デコードパターンが繰り返される。
- **改善案**: **同一ファイル内**の無名 namespace への共通ヘルパ抽出(move-only /
  extract-only)に限り実施可。公開挙動・バイト列・割当なし保証は不変。
  対象を特定できない、または効果が薄いと判断したら**実施せず「変更不要」と報告**する。
- **検証**: ホストテスト + サニタイザ + ソケット統合 + PIO ビルド。
- **リスク**: 低〜中(対象を小さく選ぶこと)。

### D2. ファイル分割 【禁止 / 提案のみ】

- CI のソース明示列挙・PIO パッケージング・README の設計哲学に関わるため、本タスクでは
  行わない。分割案(例: コアのコマンド群別分割と CI 更新手順)は報告書の提案欄に
  書くだけにする。

### D3. その他(現状維持 / 報告のみ)

- テスト体制・エラーメッセージの言語分離・ビルドフラグ設計は健全。触らない。
- `tests/slmp_minimal_tests.cpp`(2,051 行)の分割もしない(CI 列挙に関わる)。

---

## Implementation Phases

### Phase 0: 現状確認

1. `git status` 確認(クリーンでなければ停止・報告)
2. Baseline Commands を実行し、結果を記録(実行できない項目は未実施と明記)

### Phase 1: D1 の調査

1. ファイル内重複の具体箇所を列挙し、抽出可否・効果を判定
2. 「実施する/しない」の判断と理由を記録(しない判断は正当な成果である)

### Phase 2: D1 の実施(該当がある場合のみ)

1. 1 抽出ずつ → ホストテスト実行 → 次へ
2. 完了後にサニタイザ・ソケット統合・PIO ビルド

### Phase 3: 検証と報告

全 Verification Requirements を最終実行し、Reporting Format に従って報告。

---

## Verification Requirements

最終フェーズで:

- ホストテスト(可能なら ASan/UBSan ビルドでも)が全て通ること
- ソケット統合テストが通ること
- PIO ビルド(環境があれば)が通ること
- `git diff` で確認: `src/` のファイル一覧不変、公開ヘッダ無変更、
  `library.json` / `platformio.ini` / CI / `CHANGELOG.md` 無変更

---

## Reporting Format

1. **Baseline 結果**: 実行コマンドと結果(実行不能項目の明記を含む)
2. **D1 調査結果**: 重複箇所一覧 × 実施可否判断
3. **実施した抽出**(あれば): 対象と diff の要約
4. **検証結果**: 最後に実行したコマンドと結果(失敗を隠さない)
5. **提案事項**: D2(ファイル分割案)等、実装しなかった改善案
6. **Stop And Ask**: 発生した質問と停止範囲

---

## Refactor Result (2026-06-11)

### 1. Baseline 結果

- `git status --short`: クリーン。
- ホストテスト build:
  `g++ -std=c++17 -Wall -Wextra -Isrc tests/slmp_minimal_tests.cpp src/slmp_minimal.cpp src/slmp_error_codes.cpp src/slmp_error_messages.cpp src/slmp_error_messages_en.cpp src/slmp_error_messages_ja.cpp src/slmp_high_level.cpp -o %TEMP%/slmp_minimal_tests.exe`
  は成功。
- ホストテスト実行: `slmp_minimal_tests: ok`。`main()` 内の名前付きテスト関数は 33 件。
- ソケット統合: `python scripts/run_socket_integration.py --compiler g++` は成功。
  `normal` / `plc_error` / `disconnect` / `delay` / `malformed` の 5 シナリオが `ok`。
- PlatformIO: `pio` がローカル環境に存在しないため未実施。

### 2. D1 調査結果

- `src/slmp_high_level.cpp` の 2 つの `readTypedImpl` に、解析済み
  `AddressSpec` から `Value` を読み出す同一分岐が重複していた。
  公開 API・公開ヘッダ・ワイヤバイト列へ影響しない内部抽出として実施可能と判断。
- `src/slmp_minimal.cpp` には direct/random/block 系の payload encode 重複があるが、
  core 層の送信バイト列と無割当保証に近いため、本タスクでは実装せず報告のみ。
- `src/slmp_high_level.cpp` の write 系にも似た分岐はあるが、今回の最小改善としては
  read 系 1 抽出で十分と判断。

### 3. 実施した抽出

- `src/slmp_high_level.cpp` 内に内部ヘルパ `readAddressSpecValue` を追加。
- 2 つの `readTypedImpl` から、解析後の読み出し処理を `readAddressSpecValue` に集約。
- 差分概要: `src/slmp_high_level.cpp` のみ変更、30 行追加 / 71 行削除。
- `src/` のファイル追加・削除・rename は無し。公開ヘッダは未変更。

### 4. 検証結果

- 最終ホストテスト build: 成功。
- 最終ホストテスト実行: `slmp_minimal_tests: ok`。
- ASan/UBSan build:
  `g++ ... -fsanitize=address,undefined ...` は `-lasan` / `-lubsan` が見つからず
  リンク不可のため未実施。
- 最終ソケット統合: 5 シナリオすべて `ok`。
- `git diff --check`: 問題なし。
- 差分監査: 変更ファイルは `src/slmp_high_level.cpp` のみ。
  `platformio.ini` / `library.json` / `library.properties` / CI / `CHANGELOG.md` /
  公開ヘッダは未変更。

### 5. 提案事項

- D2 のファイル分割は今回未実施。将来実施する場合は、CI の g++ 明示列挙、
  PlatformIO パッケージング、README の Design Philosophy を同時に更新する必要がある。

### 6. Stop And Ask

- 発生なし。

---

## Out-of-scope Items(やらないこと)

- `src/` のファイル分割・追加・rename(提案のみ)
- 公開ヘッダ・フレームバイト列・ビルドフラグの変更
- コアパスへのヒープ割当・STL コンテナの導入
- `tests/` 既存内容・`scripts/`・`examples/` の変更
- バージョン変更、`CHANGELOG.md` 更新、レジストリ公開
- `platformio.ini` / `library.json` / CI の変更
- 実機 PLC / GX Simulator3 を使う検証
- 兄弟リポジトリの変更

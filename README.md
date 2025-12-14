# Push_swap_visualizer

ターミナルで動作する42の課題であるpush_swapという課題のvisualizerです。

## 特徴

- 🪟 **画面リサイズ対応**: ターミナルのサイズ変更に自動対応

## 動作環境

- Linux または macOS
- ncurses ライブラリ
- C コンパイラ (gcc/clang)

## インストール

### 依存関係のインストール

**Ubuntu/Debian:**
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

**macOS (Homebrew):**
```bash
brew install ncurses

### ビルド

```bash
# クローン
git clone <repository-url>
cd Push_swap_visualizer

# ビルド
make

# 実行
./visualizer
```

## 使い方

### 起動

```bash
./visualizer 2 1 3 6 5 8
```

### 操作方法

# ARGに数列をセット
ARG="4 67 3 87 23"

# ソートプログラムにARGを渡し、その出力をvisualizerに渡す
# visualizerにも同じARGを渡す必要があることに注意してください
```bash
./push_swap $ARG | ./visualizer $ARG

#　あるいは
./visualizer 5 2 1 4 3
```

実行後、ターミナルで pb や sa と入力してEnterを押すごとに画面が動きます（Ctrl+DやCtrl + Cで終了）。

### ポイント
正規化（Normalization）:

入力される数値は大きくても、画面の幅は限られています。draw_bar 関数内で (val - min) / (max - min) の計算を行い、数値の大きさに応じてバー（|）の長さを自動調整しています。これにより、負の数や大きな数も適切にグラフ化されます。

画面分割:

左側にStack A（緑）、右側にStack B（シアン）を表示するようにしています。

同期:

usleep(DELAY)を入れることで、処理が早すぎて見えない問題を防ぎ、アニメーションとして見えるようにしています。DELAYの値を変更すれば速度調整が可能です。

さらに拡張するなら
ステップ実行: usleepの代わりに getch() を使えば、スペースキーを押すたびに1手進む「デバッグモード」が作れます。

エラー処理: 入力値が数値でない場合や、重複チェックなどはこのコードでは省いています（通常はソートプログラム側で行うため）。

#### 最小画面サイズの変更

`cmd/game.h` で定義されています：

```c
enum e_window_flag {
  WINDOW_MIN_WIDTH = 32,   // 最小幅
  WINDOW_MIN_HEIGHT = 15,  // 最小高さ
  // ...
};
```

## 技術詳細

### 使用ライブラリ

- **ncurses**: ターミナルUI制御
- **libft**: カスタムC標準ライブラリ（42スタイル）

### シグナル処理

- `SIGWINCH`: ウィンドウサイズ変更を検知
- `SIGINT`: Ctrl+C による安全な終了

## 既知の制限事項

### メモリリーク警告

Valgrindでメモリチェックを行うと、ncursesライブラリ内部でメモリリークが報告されますが、これはライブラリの実装によるもので、アプリケーション側のバグではありません。詳細は `docs/ncurses-とメモリリーク.md` を参照してください。

### 画面サイズ

ゲームを快適にプレイするには、最低でも32x15文字のターミナルサイズが必要です。画面が小さすぎる場合は警告メッセージが表示されます。
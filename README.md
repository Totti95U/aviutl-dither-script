## 概要
Visual Studio Code (vscode) 上で `.dll` ファイルを使用する AviUtl スクリプトを作るためのひな形です。

右上の `Use this template` から新しくリポジトリを作成する、このリポジトリを `clone` する、または [Releases](https://github.com/Totti95U/aviutl-dll-vscode-template/releases) にある `.zip` ファイルを解凍することで、すぐに使える状態になります。

## 準備
事前準備として以下が必要です
- C/C++ コンパイラ (このプロジェクトの作成時は [i686-14.2.0-release-win32-dwarf-msvcrt-rt_v12-rev1](https://github.com/niXman/mingw-builds-binaries/releases/tag/14.2.0-rt_v12-rev1) を使用しました)
    - 32bit 版のコンパイラを使用してください (AviUtl が 32bit アプリケーションのため)
    - このプロジェクトのコンフィグをそのまま使用したい場合は、用意した MinGW-W64 のパスを `PATH` に追加し、`g++ --version` でコンパイラのバージョンが表示されることを確認してください
- vscode の拡張機能のインストール
    - C/C++ Extension Pack
    - Code Runner

## 使い方
基本的な使い方は以下の通り
1. `Use this template` するなり、 このリポジトリを `clone` するなり [Releases](https://github.com/Totti95U/aviutl-dll-vscode-template/releases) の `.zip` を解凍するなり、コピーなりする
2. `src` フォルダにある `main.cpp` を編集する
3. `main.cpp` をビルドする (デフォルトショートカット: `Ctrl + Shift + B`)
4. `src` フォルダに `main.dll` が生成される
5. `test.anm` を編集する
6. AviUtl の `script` フォルダに作成した `.dll` ファイルと `.anm` ファイルを置く
7. AviUtl を起動し、アニメーション効果から作成したスクリプトを読み込む

## 謝辞
`.dll` ファイルを作成する `test.cpp` ファイルの中身は [SEED264](https://github.com/SEED264) さんが[作成したもの](https://github.com/SEED264/AUL_DLL_Sample)を流用させていただいています。

## リンク集
- [AviUtl](http://spring-fragrance.mints.ne.jp/aviutl/)
- [Visual Studio Code - Code Editing. Redefined](https://code.visualstudio.com)
- [GitHub - niXman/mingw-builds-binaries: MinGW-W64 compiler binaries](https://github.com/niXman/mingw-builds-binaries)
- [LuaBinaries - Browse /5.1.4/Windows Libraries at SourceForge.net](https://sourceforge.net/projects/luabinaries/files/5.1.4/Windows%20Libraries/)
    - `lua5_1_4_Win32_dll8_lib.zip` を使用してください (バージョンを上げるとエラーがでます)
- [SEED264/AUL_DLL_Sample: Samples of DLL for AviUtl script](https://github.com/SEED264/AUL_DLL_Sample) 
- [AviUtlのスクリプト用にDLLつくろう #C++ - Qiita](https://qiita.com/SEED264/items/b57927e1dadb044cf614#%E3%82%B9%E3%82%AF%E3%83%AA%E3%83%97%E3%83%88%E5%81%B4%E3%81%AE%E5%87%A6%E7%90%86)
- [備忘録　AviUtlでDLLスクリプトを書く方法 #C++ - Qiita](https://qiita.com/shulmj_/items/1a8e146887b825a77785)

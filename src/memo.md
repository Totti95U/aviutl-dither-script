全体的なコード解説は [AviUtlのスクリプト用にDLLつくろう #C++ - Qiita](https://qiita.com/SEED264/items/b57927e1dadb044cf614#%E3%82%B9%E3%82%AF%E3%83%AA%E3%83%97%E3%83%88%E5%81%B4%E3%81%AE%E5%87%A6%E7%90%86) を参照

- AviUtl は 32bit アプリなので, コンパイラも 32bit 版を使う必要がある (1敗)
- mingw64 を使う場合, フォルダパスに全角文字を含むと ASCII で文字化けしてしまう (1敗)
    - "" で囲むなどすると回避できることもあるが、オプションを付けまくるとダメだった
- `.cpp` ファイル内の `luaL_Reg` は最後に `{nullptr, nullptr}` (または `NULL`) にする必要がある
- `.anm` ファイル内の `require("AUL_DLL_Sample")` は `.cpp` ファイル内で登録したモジュール名なので, `.dll` ファイルの名前自体は適当でもよい
- 改行コードを `CRLF` にしないと `.anm` ファイルのトラック回りがおかしくなる
- cpp では `整数 / 整数` は整数になる
- vector は使うな。malloc 使え

誤差伝播の kernel は次の順

0. なし
1. Sierra Lite
2. False Floyd-Steinberg
3. Atkinson
4. Floyd-Steinberg
5. Two-row Sierra
6. Burkes
7. Sierra
8. Stucki
9. Jarvis, Judice, and Ninke

減色方法については以下の論文を元に Adaptive Distributing Units (ADU) を使用した
- Pérez-Delgado, ML., Celebi, M.E. A comparative study of color quantization methods using various image quality assessment indices. Multimedia Systems 30, 40 (2024). https://doi.org/10.1007/s00530-023-01206-7

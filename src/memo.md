全体的なコード解説は [AviUtlのスクリプト用にDLLつくろう #C++ - Qiita](https://qiita.com/SEED264/items/b57927e1dadb044cf614#%E3%82%B9%E3%82%AF%E3%83%AA%E3%83%97%E3%83%88%E5%81%B4%E3%81%AE%E5%87%A6%E7%90%86) を参照

- AviUtl は 32bit アプリなので, コンパイラも 32bit 版を使う必要がある (1敗)
- mingw64 を使う場合, フォルダパスに全角文字を含むと ASCII で文字化けしてしまう (1敗)
    - "" で囲むなどすると回避できることもあるが、オプションを付けまくるとダメだった
- `.cpp` ファイル内の `luaL_Reg` は最後に `{nullptr, nullptr}` (または `NULL`) にする必要がある
- `.anm` ファイル内の `require("AUL_DLL_Sample")` は `.cpp` ファイル内で登録したモジュール名なので, `.dll` ファイルの名前自体は適当でもよい


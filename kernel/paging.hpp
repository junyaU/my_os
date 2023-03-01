#pragma once

#include <cstddef>

const size_t kPageDirectoryCount = 64;

// CR3レジスタにページテーブル設定　以降はこのページテーブルを参照するようになる
void SetupIdentityPageTable();

void InitializePaging();

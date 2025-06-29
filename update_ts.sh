#!/bin/bash

# 设置项目路径（默认为当前目录）
PROJECT_DIR=$(pwd)

# .pro 文件路径
PRO_FILE="$PROJECT_DIR/temp.pro"

# 输出 .ts 文件路径
TS_FILE="$PROJECT_DIR/translations/messages.ts"

# 输出 TSV 文件路径（修改为tsv格式）
CSV_FILE="$PROJECT_DIR/translations/strings.tsv"  # 改为tsv扩展名

# 创建 translations 目录（如果不存在）
mkdir -p "$PROJECT_DIR/translations"


# Step 1: 运行 lupdate 提取文案到 .ts 文件
echo "🔄 正在运行 lupdate 提取文案..."
lupdate -no-obsolete "$PRO_FILE" -ts "$TS_FILE"

if [ $? -ne 0 ]; then
    echo "❌ lupdate 执行失败，请确保已安装 Qt 并配置好环境变量。"
    exit 1
fi
echo "✅ 文案提取完成，输出至 $TS_FILE"

# Step 2: 提取 <source> 内容并临时保存
echo "🔄 正在提取 <source> 内容..."

TMP_SOURCE="$PROJECT_DIR/translations/.tmp_source.txt"

grep -o '<source>[^<]*</source>' "$TS_FILE" | \
  sed -E 's/<source>([^<]*)<\/source>/\1/' | \
  sort -u > "$TMP_SOURCE"

echo "📄 TMP_SOURCE 内容如下："
cat "$TMP_SOURCE"

echo "✅ 源文案提取完成，临时保存至 $TMP_SOURCE"

# Step 3: 过滤掉 TSV 中已存在的条目（修改为tsv处理）
echo " 正在过滤 TSV 中已存在的条目..."

TMP_NEW_ENTRIES="$PROJECT_DIR/translations/.tmp_new_entries.txt"

# 提取 TSV 文件中的已有英文文案（第一列）
cut -d$'\t' -f1 "$CSV_FILE" | tr -d '"' | sort -u > "$PROJECT_DIR/translations/.tmp_tsv_entries.txt"  # 改用制表符分隔

# 使用 comm 命令找出 source 中存在但 TSV 中不存在的条目
comm -23 "$TMP_SOURCE" "$PROJECT_DIR/translations/.tmp_tsv_entries.txt" > "$TMP_NEW_ENTRIES"

rm "$PROJECT_DIR/translations/.tmp_tsv_entries.txt"  # 清理临时文件

# 检查是否有新条目需要追加
if [ ! -s "$TMP_NEW_ENTRIES" ]; then
    echo " 没有新的文案需要追加。"
    rm "$TMP_SOURCE" "$TMP_NEW_ENTRIES"
    exit 0
fi

echo " 新文案条目如下："
cat "$TMP_NEW_ENTRIES"

# Step 4: 将新条目追加到 TSV（修改为tsv格式）（修改为tsv处理）
echo "🔄 正在将新文案追加到 TSV..."

awk 'NF {gsub(/\t/, "", $0); print $0 "\t"}' "$TMP_NEW_ENTRIES" >> "$CSV_FILE"  # 使用制表符分隔

rm "$TMP_SOURCE" "$TMP_NEW_ENTRIES"

echo "✅ 追加完成，结果已保存至 $CSV_FILE"
# 保持终端打开，等待用户输入
read -p " 按任意键退出..."

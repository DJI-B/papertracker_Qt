#!/bin/bash

# 设置项目路径（默认为当前目录）
PROJECT_DIR=$(pwd)

# TSV 文件路径
CSV_FILE="$PROJECT_DIR/translations/strings.tsv"

# 输出 translations 目录
TRANSLATIONS_DIR="$PROJECT_DIR/translations"

# 创建 translations 目录（如果不存在）
mkdir -p "$TRANSLATIONS_DIR"

# 清理旧的 .ts 文件
rm -f "$TRANSLATIONS_DIR"/*.ts

# 获取所有语言列名（跳过 SourceKey）
IFS=$'\t' read -ra HEADERS <<< "$(head -n1 "$CSV_FILE")"
LANGUAGES=("${HEADERS[@]:1}")  # 跳过第一列 SourceKey

echo "🌐 检测到支持的语言：${LANGUAGES[@]}"

# 为每种语言生成 .ts 文件
for lang in "${LANGUAGES[@]}"; do
    TS_FILE="$TRANSLATIONS_DIR/$lang.ts"
    echo "🔄 正在生成 $TS_FILE ..."

    # 开始写入 XML 格式
    cat > "$TS_FILE" <<EOF
<!DOCTYPE TS>
<TS version="2.1" language="$lang">
EOF

    # 提取翻译列索引
    for ((i = 0; i < ${#HEADERS[@]}; i++)); do
        if [[ "${HEADERS[i]}" == "$lang" ]]; then
            trans_col=$i
            break
        fi
    done

    # 将 TSV 中的内容插入到 <TS> 标签中
    tail -n +2 "$CSV_FILE" | while IFS=$'\t' read -r -a row; do
        # 提取源文本和翻译文本
        source="${row[0]}"
        trans="${row[$trans_col]}"

        # 移除首尾引号并转义特殊字符
        source=$(echo "$source" | sed 's/^"//;s/"$//' | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g')
        trans=$(echo "$trans" | sed 's/^"//;s/"$//' | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g')

        # 如果 source 为空则跳过
        [[ -z "$source" ]] && continue

        # 插入 context 和 message 到 ts 文件
        cat >> "$TS_FILE" <<EOF
    <context>
        <name>PaperTrackerMainWindow</name>
        <message>
            <source>$source</source>
            <translation>$trans</translation>
        </message>
    </context>
EOF
    done

    # 补全结尾标签
    cat >> "$TS_FILE" <<EOF
</TS>
EOF

    echo "✅ 已生成 $TS_FILE"
done

# 生成 .qm 文件
echo "🔄 正在使用 lrelease 生成 .qm 文件..."
lrelease "$TRANSLATIONS_DIR"/*.ts

if [ $? -ne 0 ]; then
    echo "❌ lrelease 执行失败，请确保已安装 Qt 工具。"
    exit 1
fi

echo "🎉 所有 .ts/.qm 文件已成功生成！"
read -p " 按任意键退出..."

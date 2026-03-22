此目录用于存放 240×240 像素的 BMP 格式封面图片（可选功能，当前固件版本暂未使用）。

文件命名规则：
  - 使用与 tags.json 中 "id" 字段对应的文件名
  - 格式：BMP 24位色，尺寸 240×240 像素
  - 例如：001.bmp, 002.bmp ...

转换工具推荐：
  - Windows: 画图（另存为 BMP）
  - Linux/Mac: ImageMagick: convert input.jpg -resize 240x240! cover.bmp
  - 在线转换工具: https://convertio.co

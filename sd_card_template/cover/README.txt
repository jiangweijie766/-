此目录用于存放专辑封面 BMP 图片（固件已支持，播放时在黑胶唱片中心旋转显示）。

文件命名规则：
  - 使用与 tags.json 中 "id" 字段对应的文件名
  - 格式：BMP 24位色，推荐尺寸 100×100 像素（固件加载时裁剪到 100×100）
  - 例如：001.bmp, 002.bmp ...

转换工具推荐：
  - Windows: 画图（另存为 BMP，缩放至 100×100）
  - Linux/Mac: ImageMagick: convert input.jpg -resize 100x100! 001.bmp
  - 在线转换工具: https://convertio.co

注意：
  - 仅支持 24-bit BMP（不支持 RLE 压缩或 16-bit BMP）
  - 无封面时，固件自动显示旋转色块占位图案，不影响播放

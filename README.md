

# OpenKey
### [Download bản mới nhất](https://github.com/tuyenvm/OpenKey/releases)
### Open source Vietnamese Input App for macOS - Bộ gõ tiếng Việt nguồn mở cho macOS.
New Vietnamese input for macOS. Annoying underline issue fixed. Intelligent engine. Free and open source.  
Bộ gõ tiếng Việt mới cho macOS, sử dụng kỹ thuật backkey. Loại bỏ lỗi gạch chân khó chịu ở bộ gõ mặc định. Hoàn toàn miễn phí và là nguồn mở, luôn cập nhật và phát triển.

### Lưu ý, khi sử dụng OpenKey, bạn nên tắt hẳn bộ gõ khác vì 2 chương trình bộ gõ sẽ xung đột nhau, dẫn đến thao tác không chính xác.

![Giao diện](https://github.com/tuyenvm/OpenKey/raw/master/Images/screenshot1.2.png "Main UI")
![Menu](https://github.com/tuyenvm/OpenKey/raw/master/Images/screenshot2.1.png "Menu bar")
![Gõ tắt](https://github.com/tuyenvm/OpenKey/raw/master/Images/screenshot4.png "Macro")

## Input Type - Hỗ trợ kiểu gõ
- Telex
- VNI
- Simple Telex

## Code - Bảng mã:
- Unicode (Unicode dựng sẵn).
- TCVN3 (ABC).
- VNI Windows.
- Unicode Compound (Unicode tổ hợp).
- Vietnamese Locale CP 1258.
- ...

## Feature - Tính năng:
- **Modern orthography** (On/Off) - Đặt dấu oà, uý thay vì òa, úy.
- **Quick Telex** (On/Off) - Gõ nhanh (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th).
- **Grammar check** (On/Off) - Kiểm tra ngữ pháp.
- **Spelling check** (On/Off) - Kiểm tra chính tả.
- **Restore key if invalid word** (on/off) - Phục hồi phím với từ sai.
- **Run on startup** (On/Off) - Chạy cùng macOS.
- **Gray menu bar icon** (On/Off) - Biểu tượng xám trên thanh menu phù hợp với chế độ Dark mode.
- **Switch input mode by shortcut key** - Đổi chế độ gõ bằng phím tắt tùy chọn.
- **Autocorrect fixed** (On/Off) - Sửa lỗi autocorrect trên trình duyệt như Chrome, Safari, Firefox, Microsoft Excel.
- **Underline issue fixed on macOS** (On/Off) - Sửa lỗi gạch chân trên macOS.
- **Macro** - Tính năng gõ tắt vô cùng tiện lợi. Gõ tắt của macOS chỉ hỗ trợ 20 ký tự, còn OpenKey không giới hạn ký tự.
- **Chuyển chế độ thông minh:** (On/Off) (Bản 1.2 về sau) - Bạn đang dùng chế độ gõ Tiếng Việt trên ứng dụng A, bạn chuyển qua ứng dụng B trước đó bạn dùng chế độ gõ Tiếng Anh, OpenKey sẽ tự động chuyển qua chế độ gõ Tiếng Anh cho bạn, khi bạn quay lại ứng dụng A, OpenKey tất nhiên sẽ chuyển lại chế độ gõ tiếng Việt, rất cơ động.
- **Viết Hoa chữ cái đầu câu** (On/Off) (Bản 1.2 về sau) - Khi gõ văn bản dài, đôi khi bạn quên ghi hoa chữ cái đầu câu khi kết thúc một câu hoặc khi xuống hàng, tính năng này sẽ tự ghi hoa chữ cái đầu câu cho bạn, thật tuyệt vời.
- **Chế độ “Gửi từng phím”:** (On/Off) (Bản 1.1 về sau) mặc định dùng kỹ thuật mới gửi dữ liệu 1 lần thay vì gửi nhiều lần cho chuỗi ký tự, nên nếu có ứng dụng nào không tương thích, hãy bật tính năng này lên, mặc định thì nên tắt vì kỹ thuật mới sẽ chạy nhanh hơn.
- **Cập nhật tự động:** (Bản 1.3 về sau) tính năng hỗ trợ cập nhật phiên bản OpenKey mới nhất mỗi khi mở OpenKey hoặc tự check trong phần mục Giới thiệu.
- ...


[Changelog](https://github.com/tuyenvm/OpenKey/blob/master/CHANGELOG.md)

## Cài đặt:
**Cài đặt thủ công:**  
Tải bản OpenKey mới nhất từ [đây](https://github.com/tuyenvm/OpenKey/releases), mở file `dmg` ra rồi kéo thả `OpenKey.app` vào thư mục `Application`.

**Cài bằng Homebrew:** (by nhymxu)  
Nếu chưa cài Homebrew, mở terminal, nhập:
```
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Kiểm tra phiên bản OpenKey:
```
$ brew cask info openkey
```
Gõ lệnh sau để homebrew tự cài OpenKey cho bạn:
```
$ brew cask install openkey
```

Để update phiên bản mới nhất của OpenKey
```
$ brew cask upgrade openkey
```

## Note - Lưu ý:
OpenKey needs Accessibility permission, go to System Preferences -> Security & Privacy -> Accessibility, enable OpenKey.app. Don't turn off while using.
![Guide](https://github.com/tuyenvm/OpenKey/raw/master/Images/screenshot3.png "Accessibility").

## Author
Mai Vũ Tuyên.
Any ideas, please send me via (maivutuyen.91@gmail.com)  
Fanpage: [https://www.facebook.com/OpenKeyVN](https://www.facebook.com/OpenKeyVN)

## One more thing
[Buy me a coffee ^^](https://tuyenvm.github.io)  
[Or Redbull is also Ok ^^](https://paypal.me/tuyenmai)  
Thank you very much.

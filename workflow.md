# Quy trình làm việc (Workflow)

## Ngôn ngữ và cách làm việc

- Luôn trả lời người dùng bằng tiếng Việt.
- Trước khi viết code, chỉnh sửa file, hoặc phân tích dự án, bắt buộc đọc kỹ toàn bộ file `workflow.md`.
- Luôn dùng skill `ponyman` khi code, kể cả khi người dùng không tag skill.
- Nếu người dùng chỉ hỏi ý kiến/phân tích, không tự ý sửa code.
- Khi người dùng chấp thuận sửa, làm thẳng vào hướng đã chốt, không lặp lại tư vấn dài dòng.

## Nguyên tắc code

- Ưu tiên YAGNI: không thêm abstraction, dependency, boilerplate nếu không cần.
- Dùng sẵn thư viện chuẩn, API native, hoặc dependency đã có trước khi viết code mới.
- Sửa tối thiểu, đúng vùng bị ảnh hưởng, giữ luồng hiện có.
- Không refactor rộng nếu không bắt buộc để fix lỗi.
- Không phá hành vi cũ khi sửa bug: giữ phím tắt, cấu hình, trạng thái người dùng, và các luồng xử lý hiện tại.
- Không chèn comment thừa. Chỉ thêm `ponytail:` comment khi có shortcut có chủ đích và nếu có nợ kỹ thuật thì nói rõ cách nâng cấp.
- Logic không tầm thường cần có một check nhỏ nhất có thể chạy được: test, assert demo, hoặc lệnh build bắt lỗi.

## Tiêu chuẩn giao diện

- Giữ đúng phong cách UI hiện có của dự án.
- Không thêm trang landing, hero, card trang trí, animation, hay thành phần UI mới nếu request không cần.
- Điều khiển phải rõ nghĩa, gọn, dễ scan; icon/button/checkbox/toggle/input dùng đúng vai trò.
- Không để text tràn, đè, hoặc che UI ở desktop/mobile.
- Không thêm palette màu mới nếu không cần; ưu tiên màu và spacing sẵn có.
- Thay đổi UI phải bảo toàn workflow cũ của người dùng.

## Quy trình sau khi code

- Chạy build/test đến khi không còn error và warning.
- Nếu build phát sinh lỗi/warning do thay đổi vừa làm, phải sửa tiếp đến sạch.
- Chỉ stage file đúng scope.
- Tự động chạy:
  - `git add <file liên quan>`
  - `git commit -m "<nội dung ngắn>"`
  - `git push`
- Báo lại commit hash thật sau khi push.
- Nếu không thể build, commit, hoặc push, nói rõ lệnh đã chạy và lỗi chặn.

## Git và an toàn file

- Không revert thay đổi của người dùng nếu không được yêu cầu rõ.
- Không dùng lệnh phá hủy như `git reset --hard` hoặc `git checkout --` nếu người dùng chưa yêu cầu rõ.
- Nếu worktree có file lạ của người dùng và không liên quan, bỏ qua.
- Nếu file người dùng chỉnh sửa trùng vùng cần sửa, đọc kỹ và làm cùng thay đổi đó.

## Bug gõ tiếng Việt cần chú ý

- Kiểu gõ từ bình trần: `tr7n` phải ra chữ `trên` có dấu.
- Khi người dùng gõ số thuần như `667`, kết quả mong muốn là `67`, không được biến thành số 6 kèm chữ ê có dấu mũ.
- Khi người dùng gõ ký tự Shift như `Shift+998`, kết quả mong muốn là `(*`, không được biến thành dấu ngoặc kèm chữ Ô có dấu mũ.
- Sau các ký tự ký hiệu như `^`, `&`, `*`, `(`, engine không được tiếp tục ép rule tiếng Việt lên chuỗi số/ký hiệu.
- Hướng fix ưu tiên: chặn nhỏ ở tầng xử lý phím trước khi áp biến đổi tiếng Việt, nhận diện ngữ cảnh số/ký hiệu và trả phím gốc.

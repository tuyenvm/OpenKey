# Workflow

## Ngon ngu va cach lam viec

- Luon tra loi nguoi dung bang tieng Viet.
- Truoc khi viet code, chinh sua file, hoac phan tich du an, bat buoc doc ky toan bo file `workflow.md`.
- Luon dung skill `ponyman` khi code, ke ca khi nguoi dung khong tag skill.
- Neu nguoi dung chi hoi y kien/phan tich, khong tu y sua code.
- Khi nguoi dung chap thuan sua, lam thang vao huong da chot, khong lap lai tu van dai dong.

## Nguyen tac code

- Uu tien YAGNI: khong them abstraction, dependency, boilerplate neu khong can.
- Dung san thu vien chuan, API native, hoac dependency da co truoc khi viet code moi.
- Sua toi thieu, dung vung bi anh huong, giu luong hien co.
- Khong refactor rong neu khong bat buoc de fix loi.
- Khong pha hanh vi cu khi sua bug: giu phim tat, cau hinh, trang thai nguoi dung, va cac luong xu ly hien tai.
- Khong chen comment thua. Chi them `ponytail:` comment khi co shortcut co chu dich va neu co tran ky thuat thi noi ro cach nang cap.
- Logic khong tam thuong can co mot check nho nhat co the chay duoc: test, assert demo, hoac lenh build bat loi.

## Tieu chuan giao dien

- Giu dung phong cach UI hien co cua du an.
- Khong them trang landing, hero, card trang tri, animation, hay thanh phan UI moi neu request khong can.
- Dieu khien phai ro nghia, gon, de scan; icon/button/checkbox/toggle/input dung dung vai tro.
- Khong de text tran, de, hoac che UI o desktop/mobile.
- Khong them palette mau moi neu khong can; uu tien mau va spacing san co.
- Thay doi UI phai bao toan workflow cu cua nguoi dung.

## Quy trinh sau khi code

- Chay build/test den khi khong con error va warning.
- Neu build phat sinh loi/warning do thay doi vua lam, phai sua tiep den sach.
- Chi stage file dung scope.
- Tu dong chay:
  - `git add <file lien quan>`
  - `git commit -m "<noi dung ngan>"`
  - `git push`
- Bao lai commit hash that sau khi push.
- Neu khong the build, commit, hoac push, noi ro lenh da chay va loi chan.

## Git va an toan file

- Khong revert thay doi cua nguoi dung neu khong duoc yeu cau ro.
- Khong dung lenh pha huy nhu `git reset --hard` hoac `git checkout --` neu nguoi dung chua yeu cau ro.
- Neu worktree co file la cua nguoi dung va khong lien quan, bo qua.
- Neu file nguoi dung chinh sua trung vung can sua, doc ky va lam cung thay doi do.

## Bug go tieng Viet can chu y

- Kieu go tu binh tran: `tr7n` phai ra chu `tren` co dau.
- Khi nguoi dung go so thuan nhu `667`, ket qua mong muon la `67`, khong duoc bien thanh so 6 kem chu e co dau mu.
- Khi nguoi dung go ky tu Shift nhu `Shift+998`, ket qua mong muon la `(*`, khong duoc bien thanh dau ngoac kem chu O co dau mu.
- Sau cac ky tu ky hieu nhu `^`, `&`, `*`, `(`, engine khong duoc tiep tuc ep rule tieng Viet len chuoi so/ky hieu.
- Huong fix uu tien: chan nho o tang xu ly phim truoc khi ap bien doi tieng Viet, nhan dien ngu canh so/ky hieu va tra phim goc.

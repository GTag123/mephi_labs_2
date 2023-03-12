### Shared mutex

Требуется реализовать класс-аналог `std::shared_mutex`, который позволяет нескольким потокам брать общий лок для чтения
либо эксклюзивный лок для чтения и записи. Интерфейс класса `SharedMutex` должен по аналогии с `std::shared_mutex` содержать
метода `lock, unlock, lock_shared, unlock_shared`, чтобы с этим мьютексом можно было работать через `std::lock_guard` и
`std::shared_lock`.

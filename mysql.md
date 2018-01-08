* 间隙锁

  https://dev.mysql.com/doc/refman/5.7/en/innodb-locking.html#innodb-gap-locks

  会触发间隙锁的场景：

  1. ```
     SELECT * FROM child WHERE id = 100;
     #如果id上有唯一索引，那么会使用index-record lock
     #如果id上没有索引，或者不是唯一索引，就会锁住前间隙(preceding gap)
     ```

  2. ```
     Insert Intention Locks
     mysql> CREATE TABLE child (id int(11) NOT NULL, PRIMARY KEY(id)) ENGINE=InnoDB;
     mysql> INSERT INTO child (id) values (90),(102);

     mysql> START TRANSACTION;
     mysql> SELECT * FROM child WHERE id > 100 FOR UPDATE;
     +-----+
     | id  |
     +-----+
     | 102 |
     +-----+
     此时从101-102有间隙锁
     ```

  3. ```
      A next-key lock on an index record also affects the “gap” before that index record. That is, a next-key lock is an index-record lock plus a gap lock on the gap preceding the index record. If one session has a shared or exclusive lock on record R in an index, another session cannot insert a new index record in the gap immediately before R in the index order.
      next-key锁是index—record lock加上gap lock
     ```

  4. ​


meta:
    id: glumboot
    file-extension: img
    endian: le
seq:
    - id: magic
      contents: ['glumboot']
    - id: reserved__1
      type: u1
      repeat: expr
      repeat-expr: 4
    - id: partition_count
      type: u4
    - id: crc32
      type: u4
    - id: reserved__2
      type: u1
      repeat: expr
      repeat-expr: 12
    - id: partitions
      type: glumboot_partition_type
      repeat: expr
      repeat-expr: partition_count
types:
    glumboot_partition_type:
        seq:
            - id: name
              type: strz
              encoding: ASCII
              size: 20
            - id: offset
              type: u4
            - id: size
              type: u4
            - id: flags
              type: u4

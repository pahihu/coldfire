
/* isa.c -- ISA bus emulation */
void isa_init(void);

/* ram.c -- RAM memory module 
 * 	 -- ROM memory module 
 * 	 -- SRAM memory module */
void ram_init(void);
void rom_init(void);
void sram_init(void);

/* sim_5206.c */
void sim_5206_init(void);

/* sim_5307.c */
void sim_5307_init(void);

/* timer_5206.c */
void timer_5206_init(void);

/* uart_5206.c */
void serial_5206_init(void);
char serial_getch(s16 port_number);
void serial_putch(s16 port_number, char c);



SECTIONS
{
	.text : {
		__sensor_module_start = .;
		*(.sensor_module)
		__sensor_module_end = .;
	}
}

#ifndef	__DND_H__
#define	__DND_H__

#ifdef	DEBUG
#define	DP	if(1)
#else
#define	DP	if(0)
#endif

typedef	enum	target_type
{
	TARGETS = 0,
	FILE_NAME = 1,
	STRING = 2,
	LENGTH = 3,
	SUN_AVAILABLE_TYPES = 4,
	SUN_LOAD = 5,
	SUN_DATA_LABEL = 6,
	SUN_DRAGDROP_DONE = 7,
	TEXT = 8,
	SUN_SELECTION_END = 9,
	NAME = 10,
	SUN_FILE_HOST_NAME = 11,
	SUN_ENUMERATION_COUNT = 12,
	UNKNOWN
} atom_t;

typedef	struct	supported_targets
{
	atom_t	type;
	char	*name;
	Atom	atom;
} targets_t;

typedef struct	Dnd
{
	char	*filename;
	char	*data;
	int	length;
	char	*data_label;
	char	*app_name;
	char	*host_name;
	int	enum_count;
} Dnd_t;

extern	targets_t	mytargets[];
extern	int		num_targets;

extern	void		owner(Widget widget, Time time);
extern	void		requestor(Widget widget, Atom selection, Time time);

extern	char		*get_atom_name(Atom atom);

extern	Boolean		get_data(char **data, unsigned long *length);

extern	char		*get_data_label();

extern	char		*get_name();

extern	char		*get_file_name();

#endif	__DND_H__

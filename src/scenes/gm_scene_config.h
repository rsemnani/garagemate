/*
 * The scene list. Order defines the GmScene* enum, so append rather than
 * insert when adding a scene.
 *
 * Included several times with different ADD_SCENE definitions to build the
 * enum, the handler prototypes and the handler tables from one source.
 */
ADD_SCENE(garagemate, start, Start)
ADD_SCENE(garagemate, brand, Brand)
ADD_SCENE(garagemate, frequency, Frequency)
ADD_SCENE(garagemate, name, Name)
ADD_SCENE(garagemate, pair, Pair)
ADD_SCENE(garagemate, door, Door)
ADD_SCENE(garagemate, door_menu, DoorMenu)
ADD_SCENE(garagemate, open, Open)
ADD_SCENE(garagemate, import, Import)
ADD_SCENE(garagemate, settings, Settings)
ADD_SCENE(garagemate, help, Help)

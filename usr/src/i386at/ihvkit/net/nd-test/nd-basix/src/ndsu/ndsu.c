/*
 * does the following (as root):
 *
 */
int main(int argc, char **argv)
{

	if ( argc != 2 ) {
		printf ("ndsu: usage \"ndsu <cmd>\"\n");
		return 1;
	}

	setuid(0);

	if ( system( argv[1] ) == -1 ) {
		printf("ndsu: error: could not execute command.\n");
		return 1;
	}
	return 0;

}


DROP SCHEMA if exists regstp CASCADE;
CREATE SCHEMA regstp;


CREATE TABLE regstp.config
(
  app_class character varying(32) NOT NULL,
  db_public character varying(256) NOT NULL,
  udp_port character varying(32) NOT NULL,
  pid_path character varying(32) NOT NULL,
  udp_buf_size character varying(32) NOT NULL,
  db_data character varying(32) NOT NULL,
  cycle_duration_us character varying(32) NOT NULL,
  CONSTRAINT config_pkey PRIMARY KEY (app_class)
)
WITH (
  OIDS=FALSE
);


CREATE TABLE regstp.prog
(
  id integer NOT NULL,
  name character varying(32) NOT NULL DEFAULT 'prog',--not used by regulator, use it as description
  first_step integer NOT NULL,
  repeat integer DEFAULT 1,
  repeat_infinite integer DEFAULT 0,
  CONSTRAINT prog_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
CREATE TABLE regstp.step
(
  id integer NOT NULL,
  next_id integer,
  prog_pid_id integer NOT NULL DEFAULT -1,
  prog_sem_id integer NOT NULL DEFAULT -1,
  goal real NOT NULL DEFAULT 20.0,
  duration interval DEFAULT '60',
  even_change integer DEFAULT 0,
  stop_kind character(1) DEFAULT 't'::bpchar,--by time or by goal reaching ('t' || 'g')
  CONSTRAINT step_pkey PRIMARY KEY (id),
  CONSTRAINT step_stop_kind_check CHECK (stop_kind = 't'::bpchar OR stop_kind = 'g'::bpchar)
)
WITH (
  OIDS=FALSE
);
CREATE TABLE regstp.pid
(
  id integer NOT NULL,
  mode character(1) DEFAULT 'h', --heater or cooler
  kp real DEFAULT 100,
  ki real DEFAULT 100,
  kd real DEFAULT 100,
  CONSTRAINT pid_pkey PRIMARY KEY (id),
  CONSTRAINT pid_mode_check CHECK (mode = 'h'::bpchar OR mode = 'c'::bpchar)
)
WITH (
  OIDS=FALSE
);
CREATE TABLE regstp.sem
(
  id integer NOT NULL,
  sensor_id integer NOT NULL  DEFAULT -1,
  em_id integer NOT NULL  DEFAULT -1,
  CONSTRAINT sem_pkey PRIMARY KEY (id, sensor_id, em_id)
)
WITH (
  OIDS=FALSE
);
CREATE TABLE regstp.pid_mapping
(
  prog_pid_id integer NOT NULL, 
  prog_id integer NOT NULL DEFAULT -1,
  pid_id integer NOT NULL DEFAULT -1,
  CONSTRAINT pid_mapping_pkey PRIMARY KEY (prog_id, pid_id, prog_pid_id)
)
WITH (
  OIDS=FALSE
);
CREATE TABLE regstp.sem_mapping
(
  prog_sem_id integer NOT NULL,
  prog_id integer NOT NULL DEFAULT -1,
  sem_id integer NOT NULL DEFAULT -1,
  CONSTRAINT sem_mapping_pkey PRIMARY KEY (prog_id, sem_id, prog_sem_id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regstp.sensor_mapping
(
  app_class character varying(32) NOT NULL,
  sensor_id integer NOT NULL,
  peer_id character varying(32) NOT NULL,
  remote_id integer NOT NULL,
  CONSTRAINT sensor_mapping_pkey PRIMARY KEY (app_class, sensor_id, peer_id, remote_id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regstp.em_mapping
(
  app_class character varying(32) NOT NULL,
  em_id integer NOT NULL,
  peer_id character varying(32) NOT NULL,
  remote_id integer NOT NULL,
  CONSTRAINT em_mapping_pkey PRIMARY KEY (app_class, em_id, peer_id, remote_id)
)
WITH (
  OIDS=FALSE
);

CREATE OR REPLACE FUNCTION regstp.getStepPrev(prog_id integer, step_id integer)
  RETURNS integer AS
$BODY$declare
 next_step_id integer;
 prev_step_id integer;
 first_step_id integer;
begin
 select first_step from regstp.prog where id=prog_id into next_step_id;
 if not FOUND then raise exception 'select first_step when prog_id was: % ', prog_id; end if;
 first_step_id=next_step_id;
 while 1 loop
  prev_step_id=next_step_id;
  select next_id from regstp.step where id=next_step_id into next_step_id; 
  if not FOUND then raise exception 'id loop failed when next_step_id was: % ', next_step_id;  end if;
  if next_step_id=step_id then
    return prev_step_id;
  end if;
  if next_step_id=first_step_id then
    raise exception 'previous step not found for step % ', step_id;
  end if;
 end loop;
 raise exception 'id loop failed when next_step_id was: % ', next_step_id;
end;$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
